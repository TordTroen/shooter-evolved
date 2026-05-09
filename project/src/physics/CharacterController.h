#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace JPH {
    class CharacterVirtual;
}
class PhysicsWorld;

class CharacterController {
public:
    CharacterController(PhysicsWorld& world, glm::vec3 startPos);
    ~CharacterController();

    CharacterController(const CharacterController&) = delete;
    CharacterController& operator=(const CharacterController&) = delete;

    // wishVelocity: desired horizontal velocity (XZ), vertical component ignored.
    void update(float deltaTime, glm::vec3 wishVelocity);

    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] bool isOnGround() const;
    [[nodiscard]] static float eyeHeight();

private:
    PhysicsWorld& m_world;
    std::unique_ptr<JPH::CharacterVirtual> m_character;
};
