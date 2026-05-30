#pragma once

#include "Actor.h"
#include <glm/glm.hpp>

class OscillatingActor : public Actor
{
public:
    OscillatingActor(glm::vec3 origin, glm::vec3 axis, float amplitude, float speed);

    void update(float dt) override;

private:
    glm::vec3 m_origin;
    glm::vec3 m_axis;
    float     m_amplitude;
    float     m_speed;
    float     m_time = 0.0f;
};
