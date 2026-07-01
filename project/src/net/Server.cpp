#include "Server.h"
#include "ActorState.h"
#include "BitStream.h"
#include "HitscanMath.h"
#include "LobbyRoster.h"
#include "MsgType.h"
#include "NameGenerator.h"
#include "Snapshot.h"
#include "SpawnSelector.h"

#include "../actor/Actor.h"
#include "../actor/SpawnPoint.h"
#include "../player/CharacterController.h"
#include "../scene/DemoScene.h"

#include <algorithm>
#include <iostream>
#include <cstring>

#include <glm/glm.hpp>

Server::Server(std::unique_ptr<Transport> transport)
    : m_transport(std::move(transport))
    , m_spawnSelector(std::make_unique<RandomSpawnSelector>())
    , m_nameRng(std::random_device{}())
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

    PlayerData pd;
    pd.netId = m_nextNetId;
    m_nextNetId.value++;
    // Name generation is a connection-time event, deliberately outside
    // runSimulationTick, so it does not violate §5 (pure/deterministic simulation).
    pd.name = std::string(net::player_name_at(m_nameRng()));

    // Create the controller at a placeholder position; try_respawn_player will move it
    // to a real spawn point. If no spawn points exist yet, the player connects dead.
    pd.controller = std::make_unique<CharacterController>(
        m_scene->physics(), glm::vec3(0.0f, 2.0f, 0.0f));

    if (!try_respawn_player(pd))
    {
        // No spawn points available — hold the player in dead/respawn state.
        // try_respawn_player is retried every tick until a spawn point appears.
        pd.state.isAlive = false;
        pd.state.health  = 0;
        // respawnAtTick stays 0; runSimulationTick will attempt respawn each tick.
    }

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

    // Both AssignPlayerId and the roster travel reliable-ordered, so the newly
    // connected client receives its own id first, then the full roster.
    broadcastRoster();
}

void Server::onDisconnect(ConnectionId conn)
{
    auto it = m_players.find(conn);
    if (it != m_players.end())
    {
        std::cout << "[Server] Player " << it->second.netId.value << " disconnected\n";
        m_players.erase(it);
        broadcastRoster();
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

    // Dead players cannot shoot — drop the intent entirely.
    if (!it->second.state.isAlive)
    {
        return;
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
        if (!pd.state.isAlive)   { continue; } // dead players have no hittable body

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

        // Death: only transition once (target must still be alive).
        if (target.state.health == 0 && target.state.isAlive)
        {
            target.state.isAlive = false;
            target.state.deaths++;    // victim death (server-authoritative)
            it->second.state.kills++; // killer = authenticated shooter connection (not intent.shooterId),
                                       // see the CHEAT comment above. Self-hits are skipped in the hit
                                       // loop, so every death here has a distinct player killer — there
                                       // is no world-kill/suicide case to attribute yet (deferred).
            target.respawnAtTick = m_serverTick
                + static_cast<uint32_t>(m_match.respawnSeconds * kTickRate);
            std::cout << "[Server] Player " << target.netId.value
                      << " died → respawning at tick " << target.respawnAtTick << "\n";
        }
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
        if (pd.state.isAlive)
        {
            pd.controller->simulate(tickDt, pd.latestInput);
            pd.state.position = pd.controller->position();
        }
        else
        {
            // Update the countdown HUD field for dead players.
            pd.state.respawnRemaining = (m_serverTick < pd.respawnAtTick)
                ? static_cast<float>(pd.respawnAtTick - m_serverTick) / kTickRate
                : 0.0f;

            // Attempt respawn when the timer has elapsed. If no spawn points exist yet,
            // leave the player dead and retry each tick until one becomes available.
            if (m_serverTick >= pd.respawnAtTick)
            {
                try_respawn_player(pd);
            }
        }

        // Always record the latest look/button state, even while dead, so the camera
        // orientation stays current (clients need yaw/pitch for free-look while dead).
        pd.state.yaw                    = pd.latestInput.yaw;
        pd.state.pitch                  = pd.latestInput.pitch;
        pd.state.buttons                = pd.latestInput.buttons;
        pd.state.lastProcessedInputTick = pd.latestInput.tick;
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

void Server::broadcastRoster()
{
    BitStream bs;
    auto msgType = static_cast<uint32_t>(MsgType::LobbyRoster);
    bs.serializeBits(msgType, 8);

    LobbyRoster roster;
    for (auto& [conn, pd] : m_players)
        roster.players.push_back({ pd.netId, pd.name });
    serialize(bs, roster);

    for (auto& [conn, pd] : m_players)
        sendReliable(conn, bs.bufferData(), bs.bufferBytes());
}

void Server::pushHistory(PlayerData& pd)
{
    pd.history[pd.historyHead % kHistorySize] = pd.state;
    ++pd.historyHead;
}

bool Server::try_respawn_player(PlayerData& pd)
{
    const auto& points = m_scene->spawnPoints();
    if (points.empty())
        return false;

    const SpawnPoint& sp = m_spawnSelector->select(points);

    pd.state.health           = 100;
    pd.state.isAlive          = true;
    pd.state.respawnRemaining = 0.0f;
    pd.state.position         = sp.position;
    pd.state.yaw              = sp.yaw;

    CharacterController::State cc_state;
    cc_state.position = sp.position;
    cc_state.velocity = glm::vec3(0.0f);
    pd.controller->set_state(cc_state);

    return true;
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
