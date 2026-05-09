#pragma once
#include "Actor.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <glm/glm.hpp>

namespace JPH { class BodyInterface; }
class PhysicsWorld;

class OscillatingActor : public Actor
{
public:
    OscillatingActor(const Mesh* mesh, PhysicsWorld& physics,
                     glm::vec3 origin, glm::vec3 axis, float amplitude, float speed);
    ~OscillatingActor() override;

    void update(float dt) override;

private:
    JPH::BodyInterface& m_bodyInterface;
    JPH::BodyID         m_bodyId;
    glm::vec3           m_origin;
    glm::vec3           m_axis;
    float               m_amplitude;
    float               m_speed;
    float               m_time = 0.0f;
};
