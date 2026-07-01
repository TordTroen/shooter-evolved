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
    uint8_t   buttons  = 0;   // last-known buttons for animation cues
    uint32_t  lastProcessedInputTick = 0; // server: tick of the last InputFrame applied (plan D3 / §6)
    uint32_t  fireCount              = 0; // server: monotonic shot count this session (drives remote muzzle flash)
    bool      isAlive                = true;
    float     respawnRemaining       = 0.0f; // seconds until respawn; 0 when alive or no spawn point available
    uint16_t  kills                  = 0; // server: cumulative kills this session (scoreboard)
    uint16_t  deaths                 = 0; // server: cumulative deaths this session (scoreboard)
};

void serialize(BitStream& bs, PlayerState& ps);
