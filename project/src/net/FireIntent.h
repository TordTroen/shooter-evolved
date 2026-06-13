#pragma once

#include "NetworkId.h"
#include <glm/glm.hpp>
#include <cstdint>

class BitStream;

struct FireIntent
{
    NetworkId shooterId;
    uint32_t  clientTick = 0;
    glm::vec3 origin     = glm::vec3(0.0f); // // CHEAT: client-supplied, used for lag-comp
    glm::vec3 direction  = glm::vec3(0.0f, 0.0f, -1.0f);
};

void serialize(BitStream& bs, FireIntent& fi);
