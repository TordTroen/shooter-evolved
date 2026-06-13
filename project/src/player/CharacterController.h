#pragma once

#include "../net/InputFrame.h"
#include <memory>
#include <glm/glm.hpp>

namespace JPH
{
    class CharacterVirtual;
}
class PhysicsWorld;

class CharacterController
{
public:
    CharacterController(PhysicsWorld& world, glm::vec3 startPos);
    ~CharacterController();

    CharacterController(const CharacterController&) = delete;
    CharacterController& operator=(const CharacterController&) = delete;

    // Advance the simulation by deltaTime given the provided input.
    // Used by the local client (prediction) and the server (authoritative).
    void simulate(float deltaTime, const InputFrame& input);

    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] bool isOnGround() const;
    [[nodiscard]] static float eyeHeight();

private:
    PhysicsWorld& m_world;
    std::unique_ptr<JPH::CharacterVirtual> m_character;
};
