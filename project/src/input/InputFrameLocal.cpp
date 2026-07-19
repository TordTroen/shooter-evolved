#include "../net/InputFrame.h"
#include "GamepadInput.h"
#include "../scene/Camera.h"

#include <SDL3/SDL.h>
#include <cmath>

InputFrame InputFrame::fromLocal(const bool* keys, const GamepadInput& gamepad,
                                 const Camera& camera, uint32_t tick)
{
    InputFrame f;
    f.tick  = tick;
    f.yaw   = camera.yaw();
    f.pitch = camera.pitch();

    // WASD → move vec2 (+y forward, +x right)
    glm::vec2 kbMove{};
    if (keys[SDL_SCANCODE_W]) kbMove.y += 1.0f;
    if (keys[SDL_SCANCODE_S]) kbMove.y -= 1.0f;
    if (keys[SDL_SCANCODE_D]) kbMove.x += 1.0f;
    if (keys[SDL_SCANCODE_A]) kbMove.x -= 1.0f;

    // Gamepad stick: SDL convention is up = negative Y; negate to match +y=forward.
    const glm::vec2 stickMove = { gamepad.move().x, -gamepad.move().y };
    f.move = kbMove + stickMove;
    const float len = std::sqrt(f.move.x * f.move.x + f.move.y * f.move.y);
    if (len > 1.0f)
        f.move /= len;

    if (keys[SDL_SCANCODE_SPACE] || gamepad.jump())
        f.buttons |= kButtonJump;

    if (keys[SDL_SCANCODE_R] || gamepad.reload())
        f.buttons |= kButtonReload;

    return f;
}
