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

    // Authoritative fire: applies damage + impulse to the actor. Used by the server.
    FireResult resolve(Scene& scene,
                       const glm::vec3& origin,
                       const glm::vec3& direction);

    // Read-only query: ray-casts and reports whether the ray would hit a damageable
    // actor, but does NOT apply damage or impulse. Used by the client for the
    // predicted hitmarker (cosmetic only — server is authoritative).
    FireResult query(const Scene& scene,
                     const glm::vec3& origin,
                     const glm::vec3& direction) const;

    [[nodiscard]] int damage() const { return m_damage; }

private:
    int   m_damage;
    float m_impulse;
};
