#pragma once

#include "NetworkId.h"
#include "../weapons/WeaponDef.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>

class BitStream;

// Replicated state of a floor/dropped weapon pickup. Carried in SnapshotMessage.
// Picked-up/despawned is a persistent isAlive = false field - not a one-shot event - so
// a dropped snapshot self-heals on the next one, matching ActorState (NetworkingGuidelines
// §2, §3).
struct WeaponItemState
{
    NetworkId          netId;
    weapons::WeaponId  weapon      = weapons::WeaponId::BasicPistol;
    glm::vec3          position    = glm::vec3(0.0f);
    glm::quat          rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    int32_t            ammoInMag   = 0;
    int32_t            reserveAmmo = 0;
    bool               isAlive     = true;
};

void serialize(BitStream& bs, WeaponItemState& item);
