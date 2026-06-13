#include "Server.h"
#include "BitStream.h"
#include "MsgType.h"

#include "../player/CharacterController.h"
#include "../scene/DemoScene.h"

#include <iostream>
#include <cstring>

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
            onInputFrame(from, frame);
            break;
        }
        case MsgType::FireIntentMsg:
        {
            FireIntent intent{};
            serialize(bs, intent);
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
    // TODO V1: lag-comp hitscan. For now log only.
    (void)from; (void)intent;
    std::cout << "[Server] FireIntent from shooter " << intent.shooterId.value << "\n";
}

// ---- simulation ----

void Server::runSimulationTick()
{
    const float tickDt = 1.0f / kTickRate;

    for (auto& [conn, pd] : m_players)
    {
        pd.controller->simulate(tickDt, pd.latestInput);
        pd.state.position = pd.controller->position();
        pd.state.yaw      = pd.latestInput.yaw;
        pd.state.pitch    = pd.latestInput.pitch;
        pd.state.buttons  = pd.latestInput.buttons;
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
    bs.serializeBits(m_serverTick, 32);

    auto playerCount = static_cast<uint32_t>(m_players.size());
    bs.serializeBits(playerCount, 8);

    for (auto& [conn, pd] : m_players)
    {
        bs.serializeBits(const_cast<uint32_t&>(pd.netId.value), 32);
        serialize(bs, const_cast<PlayerState&>(pd.state));
    }

    // V1: no separate fire events (attach to snapshot in V2 if needed).
    uint32_t eventCount = 0;
    bs.serializeBits(eventCount, 8);

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
