#include "PhysicsBody.h"

#include "physics/PhysicsWorld.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

PhysicsBody::PhysicsBody(PhysicsWorld& world,
                         JPH::RefConst<JPH::Shape> shape,
                         JPH::RVec3 position,
                         JPH::Quat rotation,
                         JPH::EMotionType motionType,
                         JPH::ObjectLayer layer)
    : m_bodyInterface(world.system().GetBodyInterface())
{
    JPH::BodyCreationSettings settings(shape, position, rotation, motionType, layer);
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
    m_bodyInterface.MoveKinematic(m_id, position, rotation, dt);
}
