#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

// Wraps SDL gamepad lifetime, hot-plug, axis deadzone, and per-frame state polling.
// Call handleEvent() for every SDL_Event, then update() once per frame before reading.
class GamepadInput
{
public:
    GamepadInput();
    ~GamepadInput();

    GamepadInput(const GamepadInput&) = delete;
    GamepadInput& operator=(const GamepadInput&) = delete;

    void handleEvent(const SDL_Event& event);
    void update(float deltaTime);

    [[nodiscard]] bool      fireHeld() const { return m_fireHeld; } // true while trigger is held
    [[nodiscard]] bool      jump() const { return m_jump; }  // true while A is held
    [[nodiscard]] bool      reload() const { return m_reload; } // true for one frame on X tap
    [[nodiscard]] glm::vec2 move() const { return m_move; }  // left stick, [-1,1] with deadzone
    [[nodiscard]] glm::vec2 look() const { return m_look; }  // right stick, degrees this frame

private:
    static constexpr Sint16 kTriggerThreshold = 8000;   // ~25% of 32767
    static constexpr float  kDeadzone         = 0.15f;
    static constexpr float  kLookSpeed        = 170.0f; // degrees/sec at full deflection

    SDL_Gamepad* m_gamepad     = nullptr;
    bool         m_prevWest    = false; // West (X) button, previous frame - for tap edge-detect
    bool         m_fireHeld    = false;
    bool         m_jump        = false;
    bool         m_reload      = false;
    glm::vec2    m_move        = {};
    glm::vec2    m_look        = {};

    void tryOpen(SDL_JoystickID id);
    static float deadzone(float v);
};
