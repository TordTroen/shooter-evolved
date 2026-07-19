#pragma once
#include "../weapons/WeaponDef.h"

#include <glm/glm.hpp>

class Scene;

struct FireResult
{
    bool      hit      = false;
    bool      damaged  = false;  // hit AND target had health
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal   = glm::vec3(0.0f);  // surface normal at the hit (for decal orientation)
    bool      hitActor = false;             // hit a scene Actor (player / dynamic prop), not static world
};

// Resolves a single hitscan ray (one pellet) against scene actors/world geometry, using
// the damage/impulse from a WeaponDef. Callers loop this per pellet direction (see
// weapons::pellet_directions) for shotgun-style multi-pellet weapons.
class Weapon
{
public:
    explicit Weapon(weapons::WeaponDef def = {});

    // Authoritative fire: applies damage + impulse to the actor. Used by the server.
    FireResult resolve(Scene& scene,
                       const glm::vec3& origin,
                       const glm::vec3& direction);

    // Read-only query: ray-casts and reports whether the ray would hit a damageable
    // actor, but does NOT apply damage or impulse. Used by the client for the
    // predicted hitmarker (cosmetic only - server is authoritative).
    FireResult query(const Scene& scene,
                     const glm::vec3& origin,
                     const glm::vec3& direction) const;

    [[nodiscard]] int damage() const { return m_def.damage; }
    [[nodiscard]] const weapons::WeaponDef& def() const { return m_def; }

private:
    weapons::WeaponDef m_def;
};
