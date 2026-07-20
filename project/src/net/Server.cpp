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
#include "../actor/components/PhysicsBody.h"
#include "../physics/PhysicsLayers.h"
#include "../player/CharacterController.h"
#include "../scene/DemoScene.h"
#include "../scene/Orientation.h"
#include "../weapons/Pellets.h"
#include "../weapons/PickupSelection.h"
#include "../weapons/WeaponRegistry.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>

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

    spawnFloorWeapons();
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

    pd.inventory = weapons::make_starting_inventory();

    if (!try_respawn_player(pd))
    {
        // No spawn points available - hold the player in dead/respawn state.
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

    // Vacancy-only election (never unconditional - see setLeader's doc comment).
    // setLeader() triggers its own broadcastRoster(), so the newly connected client
    // still receives its own id first, then the (leader-inclusive) roster.
    electLeaderIfVacant();
    broadcastRoster();
}

void Server::onDisconnect(ConnectionId conn)
{
    auto it = m_players.find(conn);
    if (it != m_players.end())
    {
        std::cout << "[Server] Player " << it->second.netId.value << " disconnected\n";
        m_players.erase(it);
        electLeaderIfVacant();
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
        case MsgType::RequestStartGame:
        {
            onRequestStartGame(from);
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

void Server::onRequestStartGame(ConnectionId from)
{
    if (m_gameStarted) { return; }        // idempotent against replay/late sends
    // CHEAT: leadership is derived from the authoritative connection map (isLeader),
    // never trusted from message content - this message carries no payload at all.
    if (!isLeader(from)) { return; }       // drop, non-leader request
    m_gameStarted = true;
    broadcastStartGame();
}

void Server::onFireIntent(ConnectionId from, const FireIntent& intent)
{
    auto it = m_players.find(from);
    if (it == m_players.end())
    {
        return; // unknown connection → drop
    }

    // Dead players cannot shoot - drop the intent entirely.
    if (!it->second.state.isAlive)
    {
        return;
    }

    PlayerData&           shooter_pd = it->second;
    const weapons::WeaponDef& def    = weapons::registry().def(shooter_pd.inventory.equipped_weapon());

    // Fire-rate, ammo, and reload gates - authoritative regardless of what the client
    // thinks its fire mode/ammo is (a client that spams intents gains nothing).
    if (!shooter_pd.inventory.runtime().can_fire(m_serverTick, def))
    {
        return;
    }
    shooter_pd.inventory.runtime().on_fire(m_serverTick, def);

    // CHEAT: the shooter is the authenticated connection, NOT intent.shooterId.
    // A client could put another player's NetworkId on the wire; we never trust
    // that field for identity. origin/direction are client-supplied and will be
    // validated against the lag-comp history buffer in V2.
    shooter_pd.state.fireCount++; // authoritative shot count for remote muzzle-flash replication
    const NetworkId   shooter   = shooter_pd.netId;
    const glm::vec3   origin    = intent.origin;    // CHEAT: client-supplied
    const glm::vec3   direction = glm::normalize(intent.direction); // CHEAT: client-supplied

    // Pellet directions generated once for the whole shot (authoritative RNG) - every
    // pellet is resolved against the same pattern, so a shotgun blast can't accrue
    // player damage from one random spread and world damage from another.
    Weapon shot_weapon(def);
    const std::vector<glm::vec3> pellets = weapons::pellet_directions(direction, def, m_pelletRng);

    for (const glm::vec3& pellet_dir : pellets)
    {
        // Step 1: scene (actor + world geometry) raycast → nearest actor hit.
        const RayHit scene_hit  = m_scene->physics().castRay(origin, pellet_dir);
        const float  scene_dist = scene_hit.hit
            ? glm::length(scene_hit.position - origin) : -1.0f;

        // Step 2: manual ray-vs-capsule for every other player (CharacterVirtual is
        // not in the broad phase - castRay will not hit it, so we test manually).
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
            const float t = ray_vs_capsule(origin, pellet_dir, cap_a, cap_b, kPlayerCapsuleRadius);
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
            target.state.health = std::max(0, target.state.health - def.damage);
            std::cout << "[Server] Player " << target.netId.value
                      << " hit by player " << shooter.value
                      << " → health " << target.state.health << "\n";

            // Death: only transition once (target must still be alive).
            if (target.state.health == 0 && target.state.isAlive)
            {
                target.state.isAlive = false;
                target.state.deaths++;    // victim death (server-authoritative)
                shooter_pd.state.kills++; // killer = authenticated shooter connection (not intent.shooterId),
                                           // see the CHEAT comment above. Self-hits are skipped in the hit
                                           // loop, so every death here has a distinct player killer - there
                                           // is no world-kill/suicide case to attribute yet (deferred).
                target.respawnAtTick = m_serverTick
                    + static_cast<uint32_t>(m_match.respawnSeconds * kTickRate);
                std::cout << "[Server] Player " << target.netId.value
                          << " died → respawning at tick " << target.respawnAtTick << "\n";
            }
        }
        else if (has_actor_hit)
        {
            // Actor / world hit - let Weapon::resolve apply damage and impulse.
            shot_weapon.resolve(*m_scene, origin, pellet_dir);
        }
        else
        {
            std::cout << "[Server] FireIntent from player " << shooter.value << " → miss\n";
        }
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

        // Reload/switch are discrete inputs: only act on the press-edge, not every tick
        // the button is held.
        const bool reload_pressed = (pd.latestInput.buttons & InputFrame::kButtonReload) != 0;
        const bool reload_edge    = reload_pressed && !(pd.prevButtons & InputFrame::kButtonReload);
        const bool switch_pressed = (pd.latestInput.buttons & InputFrame::kButtonSwitch) != 0;
        const bool switch_edge    = switch_pressed && !(pd.prevButtons & InputFrame::kButtonSwitch);
        pd.prevButtons = pd.latestInput.buttons;

        // Pickup/drop are holds: run every tick through the tap/hold disambiguator so a
        // short press does nothing and a sustained press fires once, on the crossing tick.
        const bool pickup_pressed = (pd.latestInput.buttons & InputFrame::kButtonPickup) != 0;
        const bool drop_pressed   = (pd.latestInput.buttons & InputFrame::kButtonDrop) != 0;
        const weapons::HoldTapEdges pickup_edges = weapons::update_hold_tap(pd.pickupHold, pickup_pressed);
        const weapons::HoldTapEdges drop_edges   = weapons::update_hold_tap(pd.dropHold, drop_pressed);

        if (pd.state.isAlive)
        {
            if (switch_edge)          { pd.inventory.switch_next(); }
            if (pickup_edges.holdStart) { tryPickupWeapon(pd); }
            if (drop_edges.holdStart)   { tryDropWeapon(pd); }
        }

        const weapons::WeaponDef& def = weapons::registry().def(pd.inventory.equipped_weapon());

        if (pd.state.isAlive && reload_edge)
        {
            pd.inventory.runtime().request_reload(m_serverTick, def);
        }
        pd.inventory.runtime().advance(m_serverTick, def);

        // Replicate weapon HUD state (magazineCapacity/reserveAmmoMax stay off the wire -
        // the HUD derives them from equippedWeapon via the registry).
        pd.state.equippedWeapon  = pd.inventory.equipped_weapon();
        pd.state.ammoInMag       = pd.inventory.runtime().ammoInMag;
        pd.state.reserveAmmo     = pd.inventory.runtime().reserveAmmo;
        pd.state.reloadRemaining = (pd.inventory.runtime().reloadFinishTick != 0)
            ? static_cast<float>(pd.inventory.runtime().reloadFinishTick - m_serverTick) / kTickRate
            : 0.0f;

        pushHistory(pd);
    }

    m_scene->tick(tickDt);
    updateWeaponItems(); // after physics integration, so dropped-item transforms are fresh
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
    // should_replicate() returned true at spawn - currently maxHealth > 0).
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

    for (const auto& item : m_weaponItems)
    {
        // Defensive cap - never write more than the wire format can carry (kMaxWeaponItems
        // is only enforced on read, so an oversized write would corrupt the whole snapshot).
        if (snap.weaponItems.size() >= static_cast<size_t>(SnapshotMessage::kMaxWeaponItems)) { break; }

        WeaponItemState ws;
        ws.netId       = item.netId;
        ws.weapon      = item.weapon;
        ws.position    = item.position;
        ws.rotation    = item.rotation;
        ws.ammoInMag   = item.ammoInMag;
        ws.reserveAmmo = item.reserveAmmo;
        ws.isAlive     = item.isAlive;
        snap.weaponItems.push_back(ws);
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
    roster.leaderId = m_leaderNetId;
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

// ---- leadership ----

void Server::setLeader(NetworkId id)
{
    m_leaderNetId = id;
    broadcastRoster();
}

void Server::electLeaderIfVacant()
{
    bool vacant = m_leaderNetId == kInvalidNetworkId;
    if (!vacant)
    {
        vacant = std::none_of(m_players.begin(), m_players.end(),
            [this](const auto& entry) { return entry.second.netId == m_leaderNetId; });
    }
    if (!vacant) { return; }

    NetworkId lowest = kInvalidNetworkId;
    for (auto& [conn, pd] : m_players)
    {
        if (lowest == kInvalidNetworkId || pd.netId.value < lowest.value)
            lowest = pd.netId;
    }
    setLeader(lowest); // lowest stays invalid if m_players is empty
}

bool Server::isLeader(ConnectionId conn) const
{
    auto it = m_players.find(conn);
    return it != m_players.end() && it->second.netId == m_leaderNetId;
}

// ---- helpers ----

void Server::sendReliable(ConnectionId conn, const std::byte* data, size_t len)
{
    m_transport->send(conn, Channel::Reliable, data, len);
}

// ---- weapon inventory / pickup / drop ----

void Server::spawnFloorWeapons()
{
    // One of each registered weapon, spread across the floor away from the demo scene's
    // boxes/spawn points. No physics body - pickup is a pure distance check,
    // and these never move, so there is nothing for a body to simulate.
    const struct { weapons::WeaponId id; glm::vec3 pos; } kFloorWeapons[] = {
        { weapons::WeaponId::BasicGun,     glm::vec3(8.0f,  0.3f,  8.0f) },
        { weapons::WeaponId::BasicPistol,  glm::vec3(-8.0f, 0.3f,  8.0f) },
        { weapons::WeaponId::BasicShotgun, glm::vec3(0.0f,  0.3f, -16.0f) },
    };

    for (const auto& fw : kFloorWeapons)
    {
        const weapons::WeaponDef& def = weapons::registry().def(fw.id);

        WeaponItem item;
        item.netId       = m_nextWeaponItemNetId;
        ++m_nextWeaponItemNetId.value;
        item.weapon      = fw.id;
        item.position    = fw.pos;
        item.ammoInMag   = def.magazineCapacity;
        item.reserveAmmo = def.reserveAmmoMax;
        item.isAlive     = true;
        item.hasLifetime = false;
        m_weaponItems.push_back(std::move(item));
    }
}

void Server::spawnDroppedWeapon(const PlayerData& pd, weapons::WeaponId weapon,
                                 const weapons::WeaponRuntime& runtime)
{
    // Throw direction: yaw-only (pitch dropped) so looking down while dropping doesn't
    // dump the item straight into the floor at the player's feet - it also gets a fixed
    // upward loft so it visibly arcs away instead of skidding along the ground.
    const glm::vec3 horizontalAim = orientation::basis_from_yaw_pitch(pd.state.yaw, 0.0f).front;
    const glm::vec3 throwDir      = glm::normalize(horizontalAim + glm::vec3(0.0f, 0.35f, 0.0f));

    // Spawn ahead of and above the player's own capsule (kPlayerCapsuleRadius = 0.3 m) so
    // the item doesn't start out overlapping the player and get shoved around by
    // depenetration before the throw impulse ever takes effect.
    const glm::vec3 spawnPos = pd.state.position
        + horizontalAim * (kPlayerCapsuleRadius + 0.5f)
        + glm::vec3(0.0f, 1.0f, 0.0f);

    WeaponItem item;
    item.netId       = m_nextWeaponItemNetId;
    ++m_nextWeaponItemNetId.value;
    item.weapon        = weapon;
    item.position       = spawnPos;
    item.ammoInMag      = runtime.ammoInMag;
    item.reserveAmmo    = runtime.reserveAmmo;
    item.isAlive        = true;
    item.hasLifetime    = true;
    item.despawnAtTick  = m_serverTick + static_cast<uint32_t>(kDropLifetimeSeconds * kTickRate);

    // Box shape sized per weapon (WeaponDef::dropBoxHalfExtents)
    const weapons::WeaponDef& weaponDef = weapons::registry().def(weapon);
    const glm::vec3&          half      = weaponDef.dropBoxHalfExtents;

    item.body = std::make_unique<PhysicsBody>(
        m_scene->physics(),
        new JPH::BoxShape(JPH::Vec3(half.x, half.y, half.z)),
        JPH::RVec3(item.position.x, item.position.y, item.position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING,
        1.0f);
    item.body->applyImpulse(throwDir * kDropThrowImpulse, item.position);

    // Random tumble - AddImpulse at the body's own position produces zero torque (no
    // lever arm), so without this the box would only ever translate and land in whatever
    // orientation it spawned in. Server-authoritative RNG (m_pelletRng) is fine to reuse
    // here since this is cosmetic tumble, not gameplay-affecting. Magnitude is per-weapon
    // (WeaponDef::dropSpinImpulse) so heavier/lighter weapons can tumble differently.
    std::uniform_real_distribution<float> spin(-weaponDef.dropSpinImpulse, weaponDef.dropSpinImpulse);
    item.body->applyAngularImpulse(glm::vec3(spin(m_pelletRng), spin(m_pelletRng), spin(m_pelletRng)));

    m_weaponItems.push_back(std::move(item));
}

Server::WeaponItem* Server::findWeaponItem(NetworkId id)
{
    for (auto& item : m_weaponItems)
    {
        if (item.netId == id) { return &item; }
    }
    return nullptr;
}

void Server::tryPickupWeapon(PlayerData& pd)
{
    std::vector<weapons::PickupCandidate> candidates;
    candidates.reserve(m_weaponItems.size());
    for (const auto& item : m_weaponItems)
    {
        if (!item.isAlive) { continue; }
        candidates.push_back({ item.netId.value, item.position });
    }

    const auto found = weapons::nearest_item_in_range(pd.state.position, candidates);
    if (!found) { return; }

    WeaponItem* item = findWeaponItem(NetworkId{ found->netId });
    if (!item) { return; } // candidate came straight from m_weaponItems - defensive only

    const weapons::WeaponDef& def = weapons::registry().def(item->weapon);

    std::optional<weapons::WeaponId> droppedWeapon;
    weapons::WeaponRuntime           droppedRuntime;
    pd.inventory.add_or_pickup(item->weapon, item->ammoInMag, item->reserveAmmo, def,
                                droppedWeapon, &droppedRuntime);

    item->isAlive    = false;
    item->deadAtTick = m_serverTick;
    item->body.reset();

    if (droppedWeapon)
    {
        spawnDroppedWeapon(pd, *droppedWeapon, droppedRuntime);
    }
}

void Server::tryDropWeapon(PlayerData& pd)
{
    weapons::WeaponId      droppedId;
    weapons::WeaponRuntime droppedRuntime;
    if (!pd.inventory.drop_selected(droppedId, droppedRuntime)) { return; } // last weapon - no-op

    spawnDroppedWeapon(pd, droppedId, droppedRuntime);
}

void Server::updateWeaponItems()
{
    static constexpr uint32_t kDeadItemPruneTicks = static_cast<uint32_t>(2.0f * kTickRate);

    for (auto& item : m_weaponItems)
    {
        // Dynamic items (dropped, not yet picked up/despawned) fall/settle under physics -
        // pull the transform back so replication reflects where it actually landed.
        if (item.body && item.isAlive)
        {
            item.position = item.body->position();
            item.rotation = item.body->rotation();
        }

        if (item.hasLifetime && item.isAlive && m_serverTick >= item.despawnAtTick)
        {
            item.isAlive    = false;
            item.deadAtTick = m_serverTick;
            item.body.reset();
        }
    }

    // Prune long-dead items so the list (and per-snapshot payload) doesn't grow
    // unbounded across a long-running match. A short grace period keeps isAlive=false
    // on the wire long enough to self-heal a dropped snapshot (NetworkingGuidelines §2/§3)
    // before the entry disappears entirely.
    std::erase_if(m_weaponItems, [this](const WeaponItem& item)
    {
        return !item.isAlive && (m_serverTick - item.deadAtTick) > kDeadItemPruneTicks;
    });
}
