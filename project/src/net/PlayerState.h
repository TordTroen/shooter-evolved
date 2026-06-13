#pragma once

#include <glm/glm.hpp>
#include <cstdint>

class BitStream;

struct PlayerState
{
    glm::vec3 position = glm::vec3(0.0f);
    float     yaw      = 0.0f;
    float     pitch    = 0.0f;
    int32_t   health   = 100;
    uint8_t   buttons  = 0; // last-known buttons for animation cues
};

void serialize(BitStream& bs, PlayerState& ps);
