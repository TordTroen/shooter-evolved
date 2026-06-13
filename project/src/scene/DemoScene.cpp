#include "DemoScene.h"

#include "actor/Actor.h"
#include "actor/OscillatingActor.h"
#include "actor/components/MeshRenderer.h"
#include "actor/components/PhysicsBody.h"
#include "physics/PhysicsLayers.h"

#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <memory>

DemoScene::DemoScene(const Mesh* planeMesh, const Mesh* boxMesh)
    : m_planeMesh(planeMesh), m_boxMesh(boxMesh)
{
}

Actor& DemoScene::spawnBox(glm::vec3 pos,
                           glm::vec3 scale,
                           glm::vec3 color,
                           int health,
                           JPH::EMotionType motion,
                           float mass)
{
    auto a       = std::make_unique<Actor>();
    a->position  = pos;
    a->scale     = scale;
    a->maxHealth = health;
    a->health    = health;
    if (m_boxMesh)
        a->meshRenderer = std::make_unique<MeshRenderer>(m_boxMesh, color);
    a->physicsBody = std::make_unique<PhysicsBody>(
        m_physics,
        new JPH::BoxShape(JPH::Vec3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f)),
        JPH::RVec3(pos.x, pos.y, pos.z),
        JPH::Quat::sIdentity(),
        motion,
        motion == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING,
        mass);
    return spawn(std::move(a));
}

void DemoScene::setup()
{
    {
        auto a   = std::make_unique<Actor>();
        a->scale = glm::vec3(50.0f, 1.0f, 50.0f);
        if (m_planeMesh)
            a->meshRenderer = std::make_unique<MeshRenderer>(
                m_planeMesh, glm::vec3(0.45f, 0.55f, 0.40f));
        a->physicsBody = std::make_unique<PhysicsBody>(
            m_physics,
            new JPH::BoxShape(JPH::Vec3(25.0f, 0.5f, 25.0f)),
            JPH::RVec3(0.0f, -0.5f, 0.0f),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING);
        spawn(std::move(a));
    }

    spawnBox(glm::vec3(-3.0f, 0.5f, -5.0f), glm::vec3(1.0f),
             glm::vec3(0.90f, 0.50f, 0.20f), 100, JPH::EMotionType::Static);

    spawnBox(glm::vec3(3.0f, 1.0f, -8.0f), glm::vec3(1.0f, 2.0f, 1.0f),
             glm::vec3(0.25f, 0.55f, 0.90f), 150, JPH::EMotionType::Static);

    spawnBox(glm::vec3(2.0f, 0.5f, -3.0f), glm::vec3(1.0f),
             glm::vec3(0.85f, 0.80f, 0.25f), 500, JPH::EMotionType::Dynamic, 25.0f);

    {
        const glm::vec3 origin(0.0f, 0.5f, -2.0f);
        auto a = std::make_unique<OscillatingActor>(
            origin, glm::vec3(1.0f, 0.0f, 0.0f), 3.0f, 1.5f);
        a->maxHealth = 75;
        a->health    = 75;
        if (m_boxMesh)
            a->meshRenderer = std::make_unique<MeshRenderer>(
                m_boxMesh, glm::vec3(0.75f, 0.30f, 0.85f));
        a->physicsBody = std::make_unique<PhysicsBody>(
            m_physics,
            new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f)),
            JPH::RVec3(origin.x, origin.y, origin.z),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Kinematic,
            Layers::MOVING);
        spawn(std::move(a));
    }

    m_physics.system().OptimizeBroadPhase();
}
