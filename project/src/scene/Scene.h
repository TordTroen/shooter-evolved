#pragma once

#include <memory>
#include <vector>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include "physics/PhysicsWorld.h"

class Actor;

class Scene
{
public:
    Scene();
    virtual ~Scene();

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    virtual void setup() {}
    virtual void tick(float deltaTime);

    [[nodiscard]] PhysicsWorld& physics() { return m_physics; }
    [[nodiscard]] const std::vector<std::unique_ptr<Actor>>& actors() const { return m_actors; }

    Actor& spawn(std::unique_ptr<Actor> actor);

    // Returns the actor whose physics body matches the given ID, or nullptr.
    [[nodiscard]] Actor* findActorByBodyID(JPH::BodyID id) const;

protected:
    PhysicsWorld                        m_physics;
    std::vector<std::unique_ptr<Actor>> m_actors;
};
