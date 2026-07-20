#include "PhysicsBody.h"

#include "physics/PhysicsWorld.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/MassProperties.h>
#include <Jolt/Physics/PhysicsSystem.h>

PhysicsBody::PhysicsBody(PhysicsWorld& world,
                         JPH::RefConst<JPH::Shape> shape,
                         JPH::RVec3 position,
                         JPH::Quat rotation,
                         JPH::EMotionType motionType,
                         JPH::ObjectLayer layer,
                         float mass)
    : m_bodyInterface(world.system().GetBodyInterface())
    , m_motionType(motionType)
{
    JPH::BodyCreationSettings settings(shape, position, rotation, motionType, layer);
    if (motionType == JPH::EMotionType::Dynamic && mass > 0.0f)
    {
        settings.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = mass;
    }
    settings.mFriction = 0.5f;
    const JPH::EActivation activation = (motionType == JPH::EMotionType::Static)
        ? JPH::EActivation::DontActivate
        : JPH::EActivation::Activate;
    m_id = m_bodyInterface.CreateAndAddBody(settings, activation);
}

PhysicsBody::~PhysicsBody()
{
    if (!m_id.IsInvalid())
    {
        m_bodyInterface.RemoveBody(m_id);
        m_bodyInterface.DestroyBody(m_id);
    }
}

void PhysicsBody::moveKinematic(JPH::RVec3 position, JPH::Quat rotation, float dt)
{
    if (m_motionType != JPH::EMotionType::Kinematic) { return; }
    m_bodyInterface.MoveKinematic(m_id, position, rotation, dt);
}

void PhysicsBody::moveKinematic(glm::vec3 position, glm::quat rotation, float dt)
{
    moveKinematic(
        JPH::RVec3(position.x, position.y, position.z),
        JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
        dt);
}

void PhysicsBody::set_motion_type(JPH::EMotionType type)
{
    // Jolt static bodies have no MotionProperties; SetMotionType asserts on them.
    // Same-type calls are also no-ops.
    if (m_motionType == type || m_motionType == JPH::EMotionType::Static) { return; }
    m_motionType = type;
    m_bodyInterface.SetMotionType(m_id, type, JPH::EActivation::Activate);
}

void PhysicsBody::applyImpulse(glm::vec3 impulse, glm::vec3 worldPos)
{
    if (m_motionType != JPH::EMotionType::Dynamic)
        return;
    m_bodyInterface.AddImpulse(
        m_id,
        JPH::Vec3(impulse.x, impulse.y, impulse.z),
        JPH::RVec3(worldPos.x, worldPos.y, worldPos.z));
}

void PhysicsBody::applyAngularImpulse(glm::vec3 impulse)
{
    if (m_motionType != JPH::EMotionType::Dynamic)
        return;
    m_bodyInterface.AddAngularImpulse(m_id, JPH::Vec3(impulse.x, impulse.y, impulse.z));
}

glm::vec3 PhysicsBody::position() const
{
    const JPH::RVec3 p = m_bodyInterface.GetPosition(m_id);
    return { static_cast<float>(p.GetX()),
             static_cast<float>(p.GetY()),
             static_cast<float>(p.GetZ()) };
}

glm::quat PhysicsBody::rotation() const
{
    const JPH::Quat q = m_bodyInterface.GetRotation(m_id);
    return { q.GetW(), q.GetX(), q.GetY(), q.GetZ() };
}
