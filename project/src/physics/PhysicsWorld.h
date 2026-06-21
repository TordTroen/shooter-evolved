#pragma once

#include "PhysicsLayers.h"
#include <Jolt/Physics/Body/BodyID.h>
#include <glm/glm.hpp>
#include <memory>

namespace JPH
{
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
}

struct RayHit
{
    bool        hit     = false;
    glm::vec3   position{};
    JPH::BodyID bodyID{};
    glm::vec3   normal{};  // world-space surface normal at the hit point (zero on miss)
};

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void update(float deltaTime);

    [[nodiscard]] RayHit castRay(glm::vec3 origin, glm::vec3 direction, float maxDistance = 1000.0f) const;

    [[nodiscard]] JPH::PhysicsSystem& system() { return *m_system; }

    [[nodiscard]] const ObjectVsBroadPhaseLayerFilterImpl& objectVsBroadPhaseLayerFilter() const
    {
        return m_objectVsBroadPhaseLayerFilter;
    }

    [[nodiscard]] const ObjectLayerPairFilterImpl& objectLayerPairFilter() const
    {
        return m_objectLayerPairFilter;
    }

    [[nodiscard]] JPH::TempAllocatorImpl& tempAllocator() { return *m_tempAllocator; }

private:
    BPLayerInterfaceImpl              m_broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl m_objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl         m_objectLayerPairFilter;

    std::unique_ptr<JPH::TempAllocatorImpl>    m_tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool>  m_jobSystem;
    std::unique_ptr<JPH::PhysicsSystem>        m_system;
};
