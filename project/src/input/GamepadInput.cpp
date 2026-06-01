#include "GamepadInput.h"

#include <cmath>

GamepadInput::GamepadInput()
{
    // Open any gamepad already connected at startup.
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids)
    {
        if (count > 0) tryOpen(ids[0]);
        SDL_free(ids);
    }
}

GamepadInput::~GamepadInput()
{
    if (m_gamepad) SDL_CloseGamepad(m_gamepad);
}

void GamepadInput::handleEvent(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_EVENT_GAMEPAD_ADDED:
            if (!m_gamepad) tryOpen(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (m_gamepad && SDL_GetGamepadID(m_gamepad) == event.gdevice.which)
            { SDL_CloseGamepad(m_gamepad); m_gamepad = nullptr; }
            break;
        default: break;
    }
}

void GamepadInput::update(float deltaTime)
{
    m_fire = false;
    m_jump = false;
    m_move = {};
    m_look = {};

    if (!m_gamepad) return;

    // Right trigger → fire (edge-detected to match single mouse-click behaviour).
    const bool triggerHeld = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > kTriggerThreshold;
    m_fire        = triggerHeld && !m_prevTrigger;
    m_prevTrigger = triggerHeld;

    // A (South) button → jump.
    m_jump = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_SOUTH) != 0;

    // Left stick → movement.
    m_move = {
        deadzone(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f),
        deadzone(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f)
    };

    // Right stick → look (degrees this frame).
    m_look = {
        deadzone(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f) * kLookSpeed * deltaTime,
        deadzone(SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f) * kLookSpeed * deltaTime
    };
}

void GamepadInput::tryOpen(SDL_JoystickID id)
{
    m_gamepad = SDL_OpenGamepad(id);
}

float GamepadInput::deadzone(float v)
{
    return std::abs(v) < kDeadzone ? 0.0f : v;
}
