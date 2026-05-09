#pragma once

#include "PhysicsLayers.h"
#include <memory>

namespace JPH
{
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
}

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void update(float deltaTime);

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
