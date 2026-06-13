#include "Client.h"
#include "BitStream.h"
#include "MsgType.h"

#include <iostream>
#include <cstring>

Client::Client(std::unique_ptr<Transport> transport)
    : m_transport(std::move(transport))
{
    m_transport->onConnect = [this](ConnectionId) {
        std::cout << "[Client] Connected to server\n";
        if (onConnected) onConnected();
    };
    m_transport->onDisconnect = [this](ConnectionId) {
        std::cout << "[Client] Disconnected from server\n";
    };
}

Client::~Client() = default;

void Client::connect(const char* host, uint16_t port)
{
    std::cout << "[Client] Connecting to " << host << ":" << port << "\n";
    m_serverConn = m_transport->connectTo(host, port);
    if (m_serverConn == kInvalidConnection)
        std::cerr << "[Client] connectTo returned invalid connection\n";
}

void Client::pump()
{
    std::vector<IncomingMessage> msgs;
    m_transport->poll(msgs);
    for (auto& msg : msgs)
    {
        if (!msg.data.empty())
            on_receive(msg.data.data(), msg.data.size());
    }
}

void Client::sendInput(const InputFrame& input)
{
    if (m_serverConn == kInvalidConnection) return;

    BitStream bs;
    auto msgType = static_cast<uint32_t>(MsgType::InputFrameMsg);
    bs.serializeBits(msgType, 8);
    serialize(bs, const_cast<InputFrame&>(input));
    m_transport->send(m_serverConn, Channel::Unreliable, bs.bufferData(), bs.bufferBytes());
}

void Client::sendFireIntent(const FireIntent& intent)
{
    if (m_serverConn == kInvalidConnection) return;

    BitStream bs;
    auto msgType = static_cast<uint32_t>(MsgType::FireIntentMsg);
    bs.serializeBits(msgType, 8);
    serialize(bs, const_cast<FireIntent&>(intent));
    m_transport->send(m_serverConn, Channel::Unreliable, bs.bufferData(), bs.bufferBytes());
}

// ---- message dispatch ----

void Client::on_receive(const std::byte* data, size_t len)
{
    if (len == 0) return;
    const auto type = static_cast<MsgType>(static_cast<uint8_t>(data[0]));
    BitStream bs(data + 1, len - 1);
    switch (type)
    {
        case MsgType::AssignPlayerId: processAssignId(bs); break;
        case MsgType::Snapshot:       processSnapshot(bs); break;
        case MsgType::StartGame:      processStartGame();  break;
        default: break;
    }
}

void Client::processAssignId(BitStream& bs)
{
    uint32_t id = 0;
    bs.serializeBits(id, 32);
    m_localPlayerId = NetworkId{id};
    std::cout << "[Client] Assigned NetworkId " << id << "\n";
}

void Client::processSnapshot(BitStream& bs)
{
    SnapshotState snap;
    bs.serializeBits(snap.tick, 32);

    uint32_t playerCount = 0;
    bs.serializeBits(playerCount, 8);

    for (uint32_t i = 0; i < playerCount; ++i)
    {
        uint32_t netIdVal = 0;
        bs.serializeBits(netIdVal, 32);
        NetworkId netId{netIdVal};

        PlayerState ps{};
        serialize(bs, ps);
        snap.players[netId] = ps;
    }

    uint32_t eventCount = 0;
    bs.serializeBits(eventCount, 8);
    // V1: fire events will be parsed here in V2.

    m_serverTick = snap.tick;

    // Keep last 3 snapshots for interpolation.
    m_snapshots.push_back(snap);
    while (m_snapshots.size() > 3)
        m_snapshots.pop_front();

    // Update remote states (latest snapshot, excluding local player).
    m_remoteStates.clear();
    for (auto& [id, ps] : snap.players)
    {
        if (id != m_localPlayerId)
            m_remoteStates[id] = ps;
    }

    if (onSnapshot) onSnapshot(snap);
}

void Client::processStartGame()
{
    std::cout << "[Client] StartGame received\n";
    if (onStartGame) onStartGame();
}
