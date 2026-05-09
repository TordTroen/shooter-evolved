#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <thread>

// --- Layer definitions ---------------------------------------------------
// Two object layers: static world geometry vs. moving bodies (player, etc).
namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING     = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr unsigned int NUM_LAYERS = 2;
}

// Maps object layers -> broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
    }
    unsigned int GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return m_objectToBroadPhase[layer];
    }
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(layer)) {
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING): return "NON_MOVING";
        case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):     return "MOVING";
        default: return "INVALID";
        }
    }
private:
    JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::NUM_LAYERS];
};

// Object layer vs. broadphase layer filter — which object layers can collide
// with which broadphase layers.
class ObjectVsBroadPhaseLayerFilterImpl final
    : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer layer1,
                       JPH::BroadPhaseLayer layer2) const override {
        switch (layer1) {
        case Layers::NON_MOVING: return layer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:     return true;
        default:                 return false;
        }
    }
};

// Object layer vs. object layer filter.
class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        switch (a) {
        case Layers::NON_MOVING: return b == Layers::MOVING;
        case Layers::MOVING:     return true;
        default:                 return false;
        }
    }
};

// Module-level singletons for filters. Jolt holds non-owning pointers to
// these for the lifetime of the PhysicsSystem, so they must outlive it.
static BPLayerInterfaceImpl              g_broadPhaseLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl g_objectVsBroadPhaseLayerFilter;
static ObjectLayerPairFilterImpl         g_objectLayerPairFilter;

static void TraceImpl(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    std::cout << "[Jolt] " << buffer << '\n';
}

PhysicsWorld::PhysicsWorld() {
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Reserve one thread for the OS / main thread.
    const int threadCount = std::max(1,
        static_cast<int>(std::thread::hardware_concurrency()) - 1);
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threadCount);

    constexpr unsigned int cMaxBodies             = 1024;
    constexpr unsigned int cNumBodyMutexes        = 0;     // 0 = use default
    constexpr unsigned int cMaxBodyPairs          = 1024;
    constexpr unsigned int cMaxContactConstraints = 1024;

    m_system = std::make_unique<JPH::PhysicsSystem>();
    m_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
                   cMaxContactConstraints,
                   g_broadPhaseLayerInterface,
                   g_objectVsBroadPhaseLayerFilter,
                   g_objectLayerPairFilter);

    m_system->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    std::cout << "Jolt initialized\n";
}

PhysicsWorld::~PhysicsWorld() {
    m_system.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsWorld::update(float deltaTime) {
    constexpr int collisionSteps = 1;
    m_system->Update(deltaTime, collisionSteps,
                     m_tempAllocator.get(), m_jobSystem.get());
}
