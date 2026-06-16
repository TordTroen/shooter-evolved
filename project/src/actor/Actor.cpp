#include "Actor.h"

#include "components/MeshRenderer.h"
#include "components/PhysicsBody.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Actor::Actor()  = default;
Actor::~Actor() = default;

glm::mat4 Actor::modelMatrix() const
{
    const glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 r = glm::mat4_cast(rotation);
    const glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
}

void Actor::takeDamage(int amount)
{
    if (!isDamageable() || m_pendingDestroy || amount <= 0)
        return;

    health = std::max(0, health - amount);
    onDamage(amount);

    if (health == 0)
    {
        onDestroyed();
        m_pendingDestroy = true;
    }
}

void Actor::markForDestruction()
{
    m_pendingDestroy = true;
}

void Actor::syncFromSnapshot(int new_health, bool is_alive)
{
    health = new_health;
    onDamage(0); // refresh mesh damage-tint from current health / maxHealth ratio
    if (!is_alive && !m_pendingDestroy)
        m_pendingDestroy = true;
}

void Actor::onDamage(int /*amount*/)
{
    // Default reaction: tint the mesh toward red based on missing health %.
    if (!meshRenderer || maxHealth <= 0)
        return;

    const float t = 1.0f - static_cast<float>(health) / static_cast<float>(maxHealth);
    meshRenderer->color = glm::mix(meshRenderer->baseColor, glm::vec3(1.0f, 0.1f, 0.1f), t);
}
