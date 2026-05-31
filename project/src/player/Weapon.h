#pragma once
#include <glm/glm.hpp>

class Scene;

struct FireResult
{
    bool      hit      = false;
    bool      damaged  = false;  // hit AND target had health
    glm::vec3 position = glm::vec3(0.0f);
};

class Weapon
{
public:
    explicit Weapon(int damage = 25, float impulse = 50.0f);

    FireResult fire(Scene& scene,
                    const glm::vec3& origin,
                    const glm::vec3& direction);

private:
    int   m_damage;
    float m_impulse;
};
