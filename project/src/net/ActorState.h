#pragma once

#include "NetworkId.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>

class BitStream;

// Replicated state of a scene actor (props, targets). Carried in SnapshotMessage.
// Destruction is a persistent isAlive = false field — not a one-shot event — so
// a dropped snapshot self-heals on the next one (NetworkingGuidelines §2, §3).
struct ActorState
{
    NetworkId netId;
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    int32_t   health   = 0;
    bool      isAlive  = true;
};

void serialize(BitStream& bs, ActorState& as);
