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
    // Full predicted state: what the ring buffer stores per tick and what reconciliation
    // rewinds to (NetworkingGuidelines §6 / plan D3).
    struct State
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
    };

    CharacterController(PhysicsWorld& world, glm::vec3 startPos);
    ~CharacterController();

    CharacterController(const CharacterController&) = delete;
    CharacterController& operator=(const CharacterController&) = delete;

    // Advance the simulation by deltaTime given the provided input.
    // Used by the local client (prediction) and the server (authoritative) — §5.
    void simulate(float deltaTime, const InputFrame& input);

    // Full state get/set for reconciliation rewind/replay (plan D3).
    [[nodiscard]] State state() const;
    void set_state(const State& s);

    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] bool isOnGround() const;
    [[nodiscard]] static float eyeHeight();

private:
    PhysicsWorld& m_world;
    std::unique_ptr<JPH::CharacterVirtual> m_character;
};
