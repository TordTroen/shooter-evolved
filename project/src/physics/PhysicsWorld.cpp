#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thread>

static void traceImpl(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    std::cout << "[Jolt] " << buffer << '\n';
}

PhysicsWorld::PhysicsWorld()
{
    JPH::RegisterDefaultAllocator();
    JPH::Trace = traceImpl;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    const int threadCount = std::max(1,
        static_cast<int>(std::thread::hardware_concurrency()) - 1);
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threadCount);

    constexpr uint32_t maxBodies             = 1024;
    constexpr uint32_t numBodyMutexes        = 0;
    constexpr uint32_t maxBodyPairs          = 1024;
    constexpr uint32_t maxContactConstraints = 1024;

    m_system = std::make_unique<JPH::PhysicsSystem>();
    m_system->Init(maxBodies, numBodyMutexes, maxBodyPairs,
                   maxContactConstraints,
                   m_broadPhaseLayerInterface,
                   m_objectVsBroadPhaseLayerFilter,
                   m_objectLayerPairFilter);

    m_system->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    std::cout << "Jolt initialized\n";
}

PhysicsWorld::~PhysicsWorld()
{
    m_system.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsWorld::update(float deltaTime)
{
    constexpr int collisionSteps = 1;
    m_system->Update(deltaTime, collisionSteps,
                     m_tempAllocator.get(), m_jobSystem.get());
}
