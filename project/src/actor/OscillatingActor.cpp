#include "OscillatingActor.h"

#include "actor/components/PhysicsBody.h"

#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>

#include <cmath>

OscillatingActor::OscillatingActor(glm::vec3 origin, glm::vec3 axis, float amplitude, float speed)
    : m_origin(origin), m_axis(axis), m_amplitude(amplitude), m_speed(speed)
{
    position = origin;
}

void OscillatingActor::update(float dt)
{
    m_time += dt;
    position = m_origin + m_axis * (m_amplitude * std::sin(m_time * m_speed));

    if (physicsBody)
    {
        const JPH::RVec3 joltPos(position.x, position.y, position.z);
        physicsBody->moveKinematic(joltPos, JPH::Quat::sIdentity(), dt);
    }
}
