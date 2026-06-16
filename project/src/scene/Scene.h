#pragma once

#include "net/NetworkId.h"
#include <memory>
#include <vector>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include "physics/PhysicsWorld.h"

class Actor;

// First NetworkId reserved for replicated scene actors (players use 1..kMaxPlayers).
static constexpr NetworkId kFirstActorNetId{0x10000};

class Scene
{
public:
    Scene();
    virtual ~Scene();

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    virtual void setup() {}
    virtual void tick(float deltaTime);

    [[nodiscard]] PhysicsWorld&       physics()       { return m_physics; }
    [[nodiscard]] const PhysicsWorld& physics() const { return m_physics; }
    [[nodiscard]] const std::vector<std::unique_ptr<Actor>>& actors() const { return m_actors; }

    Actor& spawn(std::unique_ptr<Actor> actor);

    // Returns the actor whose physics body matches the given ID, or nullptr.
    [[nodiscard]] Actor* findActorByBodyID(JPH::BodyID id) const;

    // Returns the replicated actor with the given NetworkId, or nullptr.
    // Unknown ids must be dropped by callers — never create silently (§8).
    [[nodiscard]] Actor* findActorByNetId(NetworkId id) const;

    // When false, replicated actors (netId != kInvalidNetworkId) are excluded from
    // Scene::tick's update and physics-to-transform sync. Set false on clients; they
    // receive authoritative transforms from snapshots instead.
    void setSimulateReplicated(bool simulate) { m_simulateReplicated = simulate; }

protected:
    PhysicsWorld                        m_physics;
    std::vector<std::unique_ptr<Actor>> m_actors;

private:
    // Single id-policy site: increment counter to assign the next actor NetworkId.
    // Swapping to server-assigned ids later changes only this function.
    NetworkId assign_net_id();

    NetworkId m_nextActorNetId = kFirstActorNetId;
    bool      m_simulateReplicated = true;
};
