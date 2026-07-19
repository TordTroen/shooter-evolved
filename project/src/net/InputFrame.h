#pragma once

#include <glm/glm.hpp>
#include <cstdint>

class BitStream;
class Camera;
class GamepadInput;

struct InputFrame
{
    glm::vec2 move  = {};   // [-1,1] forward/right; +y = forward, +x = strafe right
    float     yaw   = 0.0f; // camera yaw in degrees
    float     pitch = 0.0f; // camera pitch in degrees
    uint8_t   buttons = 0;  // ButtonBits bitfield
    uint32_t  tick  = 0;

    enum ButtonBits : uint8_t
    {
        kButtonJump   = 1 << 0,
        kButtonFire   = 1 << 1,
        kButtonReload = 1 << 2,
    };

    // Build an InputFrame from local keyboard/gamepad/camera state.
    static InputFrame fromLocal(const bool* keys, const GamepadInput& gamepad,
                                const Camera& camera, uint32_t tick);
};

void serialize(BitStream& bs, InputFrame& f);
