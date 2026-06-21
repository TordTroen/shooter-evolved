#include "Server.h"
#include "ActorState.h"
#include "BitStream.h"
#include "HitscanMath.h"
#include "MsgType.h"
#include "Snapshot.h"

#include "../actor/Actor.h"
#include "../player/CharacterController.h"
#include "../scene/DemoScene.h"

#include <algorithm>
#include <iostream>
#include <cstring>

#include <glm/glm.hpp>

Server::Server(std::unique_ptr<Transport> transport)
    : m_transport(std::move(transport))
{
    m_transport->onConnect    = [this](ConnectionId c) { onConnect(c); };
    m_transport->onDisconnect = [this](ConnectionId c) { onDisconnect(c); };

    // Server-side physics scene: no meshes, physics-only.
    m_scene = std::make_unique<DemoScene>(nullptr, nullptr);
    m_scene->setup();
}

Server::~Server() = default;

void Server::start(uint16_t port)
{
    m_transport->startServer(port);
}

void Server::tick(float dt)
{
    // Drain incoming messages.
    std::vector<IncomingMessage> msgs;
    m_transport->poll(msgs);
    for (auto& msg : msgs)
    {
        if (!msg.data.empty())
            dispatch(msg.from, msg.data.data(), msg.data.size());
    }

    // Fixed-rate simulation.
    m_tickAccum += dt;
    const float tickDt = 1.0f / kTickRate;
    while (m_tickAccum >= tickDt)
    {
        m_tickAccum -= tickDt;
        runSimulationTick();
    }

    // Snapshot broadcast.
    m_snapshotAccum += dt;
    if (m_snapshotAccum >= 1.0f / kSnapshotRate)
    {
        m_snapshotAccum -= 1.0f / kSnapshotRate;
        broadcastSnapshot();
    }
}

// ---- connection events ----

void Server::onConnect(ConnectionId conn)
{
    if (m_players.size() >= static_cast<size_t>(kMaxPlayers))
    {
        std::cout << "[Server] Rejecting connection " << conn << ": server full\n";
        m_transport->disconnect(conn);
        return;
    }

    const glm::vec3 spawnPos(0.0f, 2.0f, 8.0f);
    PlayerData pd;
    pd.netId      = m_nextNetId;
    m_nextNetId.value++;
    pd.state.position = spawnPos;
    pd.state.health   = 100;
    pd.controller     = std::make_unique<CharacterController>(m_scene->physics(), spawnPos);
    pd.history.fill(pd.state);
    m_players[conn] = std::move(pd);

    std::cout << "[Server] Player " << m_players[conn].netId.value
              << " connected (conn " << conn << ")\n";

    // Assign the client its NetworkId.
    BitStream bs;
    auto msgType = static_cast<uint32_t>(MsgType::AssignPlayerId);
    bs.serializeBits(msgType, 8);
    bs.serializeBits(m_players[conn].netId.value, 32);
    sendReliable(conn, bs.bufferData(), bs.bufferBytes());
}

void Server::onDisconnect(ConnectionId conn)
{
    auto it = m_players.find(conn);
    if (it != m_players.end())
    {
        std::cout << "[Server] Player " << it->second.netId.value << " disconnected\n";
        m_players.erase(it);
    }
}

// ---- message dispatch ----

void Server::dispatch(ConnectionId from, const std::byte* data, size_t len)
{
    if (len == 0) return;
    const auto type = static_cast<MsgType>(static_cast<uint8_t>(data[0]));
    BitStream bs(data + 1, len - 1);
    switch (type)
    {
        case MsgType::InputFrameMsg:
        {
            InputFrame frame{};
            serialize(bs, frame);
            if (bs.hasError()) { break; } // CHEAT: drop malformed input, never act on it
            onInputFrame(from, frame);
            break;
        }
        case MsgType::FireIntentMsg:
        {
            FireIntent intent{};
            serialize(bs, intent);
            if (bs.hasError()) { break; } // CHEAT: drop malformed intent
            onFireIntent(from, intent);
            break;
        }
        default: break;
    }
}

void Server::onInputFrame(ConnectionId from, const InputFrame& input)
{
    auto it = m_players.find(from);
    if (it != m_players.end())
        it->second.latestInput = input;
}

void Server::onFireIntent(ConnectionId from, const FireIntent& intent)
{
    auto it = m_players.find(from);
    if (it == m_players.end())
    {
        return; // unknown connection → drop
    }

    // CHEAT: the shooter is the authenticated connection, NOT intent.shooterId.
    // A client could put another player's NetworkId on the wire; we never trust
    // that field for identity. origin/direction are client-supplied and will be
    // validated against the lag-comp history buffer in V2.
    it->second.state.fireCount++; // authoritative shot count for remote muzzle-flash replication
    const NetworkId   shooter   = it->second.netId;
    const glm::vec3   origin    = intent.origin;    // CHEAT: client-supplied
    const glm::vec3   direction = glm::normalize(intent.direction); // CHEAT: client-supplied

    // Step 1: scene (actor + world geometry) raycast → nearest actor hit.
    const RayHit scene_hit  = m_scene->physics().castRay(origin, direction);
    const float  scene_dist = scene_hit.hit
        ? glm::length(scene_hit.position - origin) : -1.0f;

    // Step 2: manual ray-vs-capsule for every other player (CharacterVirtual is
    // not in the broad phase — castRay will not hit it, so we test manually).
    bool         has_player_hit   = false;
    ConnectionId hit_player_conn  = kInvalidConnection;
    float        player_dist      = -1.0f;

    for (auto& [conn, pd] : m_players)
    {
        if (pd.netId == shooter) { continue; } // don't shoot yourself

        glm::vec3 cap_a;
        glm::vec3 cap_b;
        player_capsule_endpoints(pd.state.position, cap_a, cap_b);
        const float t = ray_vs_capsule(origin, direction, cap_a, cap_b, kPlayerCapsuleRadius);
        if (t >= 0.0f && (!has_player_hit || t < player_dist))
        {
            has_player_hit  = true;
            player_dist     = t;
            hit_player_conn = conn;
        }
    }

    // Step 3: whichever valid hit is closer wins.
    const bool has_actor_hit = scene_dist >= 0.0f;

    if (has_player_hit && (!has_actor_hit || player_dist <= scene_dist))
    {
        // Player hit.
        PlayerData& target = m_players[hit_player_conn];
        target.state.health = std::max(0, target.state.health - m_weapon.damage());
        std::cout << "[Server] Player " << target.netId.value
                  << " hit by player " << shooter.value
                  << " → health " << target.state.health << "\n";
    }
    else if (has_actor_hit)
    {
        // Actor / world hit — let Weapon::resolve apply damage and impulse.
        m_weapon.resolve(*m_scene, origin, direction);
    }
    else
    {
        std::cout << "[Server] FireIntent from player " << shooter.value << " → miss\n";
    }
}

// ---- simulation ----

void Server::runSimulationTick()
{
    const float tickDt = 1.0f / kTickRate;

    for (auto& [conn, pd] : m_players)
    {
        pd.controller->simulate(tickDt, pd.latestInput);
        pd.state.position               = pd.controller->position();
        pd.state.yaw                    = pd.latestInput.yaw;
        pd.state.pitch                  = pd.latestInput.pitch;
        pd.state.buttons                = pd.latestInput.buttons;
        pd.state.lastProcessedInputTick = pd.latestInput.tick; // for client reconciliation (plan D3)
        pushHistory(pd);
    }

    m_scene->tick(tickDt);
    ++m_serverTick;
}

void Server::broadcastSnapshot()
{
    BitStream bs;
    auto msgType = static_cast<uint32_t>(MsgType::Snapshot);
    bs.serializeBits(msgType, 8);

    SnapshotMessage snap;
    snap.serverTick = m_serverTick;
    for (auto& [conn, pd] : m_players)
        snap.players.push_back({ pd.netId, pd.state });

    // Replicated actor state: all actors with a valid NetworkId (i.e. those for which
    // should_replicate() returned true at spawn — currently maxHealth > 0).
    for (const auto& actor : m_scene->actors())
    {
        if (actor->netId == kInvalidNetworkId) { continue; }

        ActorState as;
        as.netId    = actor->netId;
        as.position = actor->position;
        as.rotation = actor->rotation;
        as.health   = actor->health;
        as.isAlive  = !actor->isPendingDestroy();
        snap.actors.push_back(as);
    }

    serialize(bs, snap);

    const size_t bytes = bs.bufferBytes();
    for (auto& [conn, pd] : m_players)
        m_transport->send(conn, Channel::Unreliable, bs.bufferData(), bytes);
}

void Server::pushHistory(PlayerData& pd)
{
    pd.history[pd.historyHead % kHistorySize] = pd.state;
    ++pd.historyHead;
}

void Server::broadcastStartGame()
{
    BitStream bs;
    auto msgType = static_cast<uint32_t>(MsgType::StartGame);
    bs.serializeBits(msgType, 8);
    for (auto& [conn, pd] : m_players)
        sendReliable(conn, bs.bufferData(), bs.bufferBytes());
    std::cout << "[Server] Broadcast StartGame to " << m_players.size() << " clients\n";
}

// ---- helpers ----

void Server::sendReliable(ConnectionId conn, const std::byte* data, size_t len)
{
    m_transport->send(conn, Channel::Reliable, data, len);
}
