#pragma once

#include "net/NetworkId.h"
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class MeshRenderer;
class PhysicsBody;
class Scene;

class Actor
{
public:
    Actor();
    virtual ~Actor();

    Actor(const Actor&)            = delete;
    Actor& operator=(const Actor&) = delete;

    virtual void update(float /*dt*/) {}

    [[nodiscard]] glm::mat4 modelMatrix() const;

    // Damage / destruction.
    // An actor with maxHealth == 0 is invincible (takeDamage is a no-op).
    void takeDamage(int amount);
    [[nodiscard]] bool isDamageable() const { return maxHealth > 0; }
    [[nodiscard]] bool isPendingDestroy() const { return m_pendingDestroy; }
    void markForDestruction();

    // Apply authoritative state received from a server snapshot (client-side only).
    // Sets health and refreshes the damage tint; marks for destruction when isAlive = false.
    void syncFromSnapshot(int new_health, bool is_alive);

    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale    = glm::vec3(1.0f);

    int maxHealth = 0;
    int health    = 0;

    // Assigned by Scene::spawn for replicated actors; kInvalidNetworkId for local-only.
    NetworkId netId;

    std::unique_ptr<MeshRenderer> meshRenderer;
    std::unique_ptr<PhysicsBody>  physicsBody;

    Scene* scene = nullptr;

protected:
    // Hooks for subclasses. Defaults: tint mesh toward red as health drops.
    virtual void onDamage(int amount);
    virtual void onDestroyed() {}

private:
    bool m_pendingDestroy = false;
};
