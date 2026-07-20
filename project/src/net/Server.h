#pragma once

#include "NetworkId.h"
#include "InputFrame.h"
#include "PlayerState.h"
#include "FireIntent.h"
#include "Transport.h"
#include "WeaponItemState.h"
#include "../core/MatchSettings.h"
#include "../player/Weapon.h"
#include "../weapons/HoldTap.h"
#include "../weapons/Inventory.h"
#include "../weapons/WeaponDef.h"
#include "../weapons/WeaponRuntime.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class DemoScene;
class CharacterController;
class PhysicsBody;
class SpawnSelector;

class Server
{
public:
    static constexpr float kTickRate     = 60.0f;
    static constexpr float kSnapshotRate = 20.0f;
    static constexpr int   kMaxPlayers   = 4;
    static constexpr int   kHistorySize  = 12; // ~200 ms at 60 Hz, for lag-comp

    static constexpr float kDropThrowImpulse    = 4.0f;  // forward along aim, dropped weapons
    static constexpr float kDropLifetimeSeconds = 30.0f; // dropped items despawn after this

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

        weapons::Inventory     inventory;
        uint8_t                prevButtons = 0; // for reload/switch press-edge detection
        weapons::HoldTapState  pickupHold;       // kButtonPickup hold/tap disambiguation
        weapons::HoldTapState  dropHold;         // kButtonDrop hold/tap disambiguation
    };

    // Server-owned replicated weapon pickup/drop. Scene-placed floor weapons have
    // hasLifetime = false (never despawn); dropped weapons carry a dynamic physics body
    // and despawn after kDropLifetimeSeconds (weapons::kDropLifetimeSeconds).
    struct WeaponItem
    {
        NetworkId          netId;
        weapons::WeaponId  weapon;
        glm::vec3          position;
        glm::quat          rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        int                ammoInMag   = 0;
        int                reserveAmmo = 0;
        bool               isAlive     = true;
        bool               hasLifetime = false;
        uint32_t           despawnAtTick = 0;
        uint32_t           deadAtTick    = 0; // tick isAlive went false; drives pruning
        std::unique_ptr<PhysicsBody> body;    // null for floor-placed (non-physics) items
    };

    std::unique_ptr<Transport>                   m_transport;
    std::unordered_map<ConnectionId, PlayerData> m_players;
    std::unique_ptr<DemoScene>                   m_scene;
    MatchSettings                                m_match;
    std::unique_ptr<SpawnSelector>               m_spawnSelector;

    uint32_t     m_serverTick    = 0;
    float        m_tickAccum     = 0.0f;
    float        m_snapshotAccum = 0.0f;
    NetworkId    m_nextNetId{1};
    std::mt19937 m_nameRng;
    std::mt19937 m_pelletRng{ std::random_device{}() }; // authoritative shotgun pellet pattern

    std::vector<WeaponItem> m_weaponItems;
    NetworkId                m_nextWeaponItemNetId{0x20000}; // separate id space from actors (0x10000+)

    NetworkId    m_leaderNetId{}; // 0/invalid = no leader yet; only setLeader() writes this
    bool         m_gameStarted = false;

    void onConnect(ConnectionId conn);
    void onDisconnect(ConnectionId conn);

    void dispatch(ConnectionId from, const std::byte* data, size_t len);
    void onInputFrame(ConnectionId from, const InputFrame& input);
    void onFireIntent(ConnectionId from, const FireIntent& intent);
    void onRequestStartGame(ConnectionId from);

    // Single choke point for leadership assignment - every leader-change mechanism
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
    // spawn points exist - caller retries on subsequent ticks (see §4 of the plan).
    bool try_respawn_player(PlayerData& pd);

    void sendReliable(ConnectionId conn, const std::byte* data, size_t len);

    // ---- weapon inventory / pickup / drop (MultipleWeapons.md) ----

    // One of each registry weapon, placed on the floor, hasLifetime = false (never despawn).
    void spawnFloorWeapons();

    // Spawns a replicated dynamic weapon item at pd's position, thrown along its aim.
    void spawnDroppedWeapon(const PlayerData& pd, weapons::WeaponId weapon,
                             const weapons::WeaponRuntime& runtime);

    // kButtonPickup holdStart: nearest in-range alive item is picked up (or its ammo
    // tops up an already-owned weapon; or, if the inventory is full, the selected
    // weapon is dropped first and the new one takes its place).
    void tryPickupWeapon(PlayerData& pd);

    // kButtonDrop holdStart: drops the selected weapon as a physics item. No-op when
    // only one weapon is held (decision #1).
    void tryDropWeapon(PlayerData& pd);

    // Marks lifetime-limited items dead once their despawn tick passes, and prunes
    // long-dead items from m_weaponItems so the list doesn't grow unbounded.
    void updateWeaponItems();

    WeaponItem* findWeaponItem(NetworkId id);
};
