#pragma once

#include "NetworkId.h"
#include "InputFrame.h"
#include "PlayerState.h"
#include "FireIntent.h"
#include "Transport.h"
#include "../core/MatchSettings.h"
#include "../player/Weapon.h"

#include <array>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

class DemoScene;
class CharacterController;
class SpawnSelector;

class Server
{
public:
    static constexpr float kTickRate     = 60.0f;
    static constexpr float kSnapshotRate = 20.0f;
    static constexpr int   kMaxPlayers   = 4;
    static constexpr int   kHistorySize  = 12; // ~200 ms at 60 Hz, for lag-comp

    explicit Server(std::unique_ptr<Transport> transport);
    ~Server();

    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;

    void start(uint16_t port);
    void tick(float dt); // Called by Net::pump() each game loop iteration.
    void broadcastStartGame(); // Reliable broadcast → clients transition to PlayingState.

    // Read-only accessors for operator-facing UI (e.g. HeadlessServer's status line).
    NetworkId leaderNetId() const { return m_leaderNetId; }
    int       playerCount() const { return static_cast<int>(m_players.size()); }
    uint32_t  currentTick()  const { return m_serverTick; }

private:
    struct PlayerData
    {
        NetworkId                             netId;
        std::string                           name;
        PlayerState                           state;
        InputFrame                            latestInput;
        std::unique_ptr<CharacterController>  controller;
        std::array<PlayerState, kHistorySize> history;
        int                                   historyHead  = 0;
        uint32_t                              respawnAtTick = 0; // valid only while !state.isAlive
    };

    std::unique_ptr<Transport>                   m_transport;
    std::unordered_map<ConnectionId, PlayerData> m_players;
    std::unique_ptr<DemoScene>                   m_scene;
    Weapon                                       m_weapon; // authoritative hitscan resolver
    MatchSettings                                m_match;
    std::unique_ptr<SpawnSelector>               m_spawnSelector;

    uint32_t     m_serverTick    = 0;
    float        m_tickAccum     = 0.0f;
    float        m_snapshotAccum = 0.0f;
    NetworkId    m_nextNetId{1};
    std::mt19937 m_nameRng;

    NetworkId    m_leaderNetId{}; // 0/invalid = no leader yet; only setLeader() writes this
    bool         m_gameStarted = false;

    void onConnect(ConnectionId conn);
    void onDisconnect(ConnectionId conn);

    void dispatch(ConnectionId from, const std::byte* data, size_t len);
    void onInputFrame(ConnectionId from, const InputFrame& input);
    void onFireIntent(ConnectionId from, const FireIntent& intent);
    void onRequestStartGame(ConnectionId from);

    // Single choke point for leadership assignment — every leader-change mechanism
    // (vacancy election now, manual transfer/vote later) must call this, never assign
    // m_leaderNetId inline elsewhere.
    void setLeader(NetworkId id);
    // Elects the lowest-netId connected player as leader, but only if the current
    // leader is unset or no longer connected. Never recomputes unconditionally, so a
    // future manual transfer isn't silently overwritten by the next connect/disconnect.
    void electLeaderIfVacant();
    bool isLeader(ConnectionId conn) const;

    void runSimulationTick();
    void broadcastSnapshot();
    void broadcastRoster();
    void pushHistory(PlayerData& pd);

    // Spawn pd at a random spawn point. Returns false and leaves pd unchanged if no
    // spawn points exist — caller retries on subsequent ticks (see §4 of the plan).
    bool try_respawn_player(PlayerData& pd);

    void sendReliable(ConnectionId conn, const std::byte* data, size_t len);
};
