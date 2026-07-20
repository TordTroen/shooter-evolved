#pragma once

#include "ActorState.h"
#include "NetworkId.h"
#include "InputFrame.h"
#include "FireIntent.h"
#include "LobbyRoster.h"
#include "PlayerState.h"
#include "Transport.h"
#include "WeaponItemState.h"

#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

struct SnapshotState
{
    uint32_t                                   tick = 0;
    std::unordered_map<NetworkId, PlayerState> players;
    std::vector<ActorState>                    actors;      // replicated scene props
    std::vector<WeaponItemState>               weaponItems; // floor pickups + dropped weapons
};

// Connection lifecycle as observed by the local client. `Connecting` covers the
// window between connect() and the transport's first status callback;
// `Failed` distinguishes "never connected" from "was connected, then dropped"
// (both ride the same transport onDisconnect - see NetworkingGuidelines §7).
enum class ConnState { Connecting, Connected, Failed };

class Client
{
public:
    explicit Client(std::unique_ptr<Transport> transport);
    ~Client();

    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;

    void connect(const char* host, uint16_t port);
    void pump(); // Drain incoming messages; call once per game loop.

    void sendInput(const InputFrame& input);
    void sendFireIntent(const FireIntent& intent);
    void requestStartGame(); // sends MsgType::RequestStartGame, Channel::Reliable, no payload

    NetworkId localPlayerId() const { return m_localPlayerId; }
    ConnState  connectionState() const { return m_connState; }

    // Latest interpolated remote states (excludes the local player).
    const std::unordered_map<NetworkId, PlayerState>& remoteStates() const
    {
        return m_remoteStates;
    }

    // Latest lobby roster (join order preserved for display).
    const std::vector<RosterEntry>& roster() const { return m_roster; }

    // Server-authoritative party leader; clients never compute this themselves.
    NetworkId leaderId() const { return m_leaderId; }
    bool isLeader() const { return leaderId() == localPlayerId(); }

    // Hooks set by owning state/system.
    std::function<void()>                  onConnected;
    std::function<void(const SnapshotState&)> onSnapshot;
    std::function<void()>                  onStartGame;
    std::function<void()>                  onRosterChanged;
    // Fires when the transport reports disconnect, whether that's a failed
    // connection attempt or a mid-game drop; connectionState() tells the caller which.
    std::function<void()>                  onDisconnected;

private:
    std::unique_ptr<Transport> m_transport;
    ConnectionId               m_serverConn   = kInvalidConnection;
    NetworkId                  m_localPlayerId{};
    uint32_t                   m_serverTick   = 0;
    ConnState                  m_connState    = ConnState::Connecting;

    // Last 3 snapshots kept for interpolation.
    std::deque<SnapshotState>                m_snapshots;
    std::unordered_map<NetworkId, PlayerState> m_remoteStates;
    std::vector<RosterEntry>                 m_roster;
    NetworkId                                m_leaderId{};

    void on_receive(const std::byte* data, size_t len);
    void processAssignId(BitStream& bs);
    void processSnapshot(BitStream& bs);
    void processStartGame();
    void processRoster(BitStream& bs);
};
