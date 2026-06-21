#include "Weapon.h"

#include "actor/Actor.h"
#include "actor/components/PhysicsBody.h"
#include "physics/PhysicsWorld.h"
#include "scene/Scene.h"

#include <iostream>

Weapon::Weapon(int damage, float impulse)
    : m_damage(damage)
    , m_impulse(impulse)
{
}

FireResult Weapon::resolve(Scene& scene, const glm::vec3& origin, const glm::vec3& direction)
{
    FireResult result;
    const RayHit hit = scene.physics().castRay(origin, direction);
    if (!hit.hit)
        return result;

    result.hit      = true;
    result.position = hit.position;
    result.normal   = hit.normal;

    std::cout << "[Shot] hit at ("
              << hit.position.x << ", "
              << hit.position.y << ", "
              << hit.position.z << ")\n";

    if (Actor* target = scene.findActorByBodyID(hit.bodyID))
    {
        result.hitActor = target->physicsBody && !target->physicsBody->isStatic();
        if (target->isDamageable())
        {
            target->takeDamage(m_damage);
            std::cout << "[Damage] " << m_damage << " dmg -> "
                      << target->health << "/" << target->maxHealth
                      << (target->isPendingDestroy() ? " (destroyed)" : "")
                      << "\n";
            result.damaged = true;
        }
        if (target->physicsBody)
        {
            target->physicsBody->applyImpulse(direction * m_impulse, hit.position);
        }
    }

    return result;
}

FireResult Weapon::query(const Scene& scene, const glm::vec3& origin, const glm::vec3& direction) const
{
    FireResult result;
    // CHEAT: client-side read-only ray query for the predicted hitmarker.
    // No damage or impulse applied — server is the only mutator (NetworkingGuidelines §1).
    const RayHit hit = scene.physics().castRay(origin, direction);
    if (!hit.hit)
        return result;

    result.hit      = true;
    result.position = hit.position;
    result.normal   = hit.normal;

    if (const Actor* target = scene.findActorByBodyID(hit.bodyID))
    {
        result.damaged  = target->isDamageable();
        // Only suppress decals for bodies that can move (dynamic or kinematic).
        // Static actors (floor, walls, static boxes) are safe to decal.
        result.hitActor = target->physicsBody && !target->physicsBody->isStatic();
    }

    return result;
}
