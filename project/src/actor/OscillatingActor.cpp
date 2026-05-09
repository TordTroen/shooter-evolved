#include "OscillatingActor.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsLayers.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

OscillatingActor::OscillatingActor(const Mesh* mesh, PhysicsWorld& physics,
                                   glm::vec3 origin, glm::vec3 axis, float amplitude, float speed)
    : Actor(mesh)
    , m_bodyInterface(physics.system().GetBodyInterface())
    , m_origin(origin), m_axis(axis), m_amplitude(amplitude), m_speed(speed)
{
    JPH::BodyCreationSettings settings(
        new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f)),
        JPH::RVec3(origin.x, origin.y, origin.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Kinematic,
        Layers::MOVING);
    m_bodyId = m_bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);
    model    = glm::translate(glm::mat4(1.0f), origin);
}

OscillatingActor::~OscillatingActor()
{
    m_bodyInterface.RemoveBody(m_bodyId);
    m_bodyInterface.DestroyBody(m_bodyId);
}

void OscillatingActor::update(float dt)
{
    m_time += dt;
    const glm::vec3  pos    = m_origin + m_axis * (m_amplitude * std::sin(m_time * m_speed));
    const JPH::RVec3 joltPos(pos.x, pos.y, pos.z);
    m_bodyInterface.MoveKinematic(m_bodyId, joltPos, JPH::Quat::sIdentity(), dt);
    model = glm::translate(glm::mat4(1.0f), pos);
}
