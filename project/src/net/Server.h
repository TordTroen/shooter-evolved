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

private:
    struct PlayerData
    {
        NetworkId                             netId;
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

    uint32_t  m_serverTick    = 0;
    float     m_tickAccum     = 0.0f;
    float     m_snapshotAccum = 0.0f;
    NetworkId m_nextNetId{1};

    void onConnect(ConnectionId conn);
    void onDisconnect(ConnectionId conn);

    void dispatch(ConnectionId from, const std::byte* data, size_t len);
    void onInputFrame(ConnectionId from, const InputFrame& input);
    void onFireIntent(ConnectionId from, const FireIntent& intent);

    void runSimulationTick();
    void broadcastSnapshot();
    void pushHistory(PlayerData& pd);

    // Spawn pd at a random spawn point. Returns false and leaves pd unchanged if no
    // spawn points exist — caller retries on subsequent ticks (see §4 of the plan).
    bool try_respawn_player(PlayerData& pd);

    void sendReliable(ConnectionId conn, const std::byte* data, size_t len);
};
