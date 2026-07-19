#pragma once

#include "WeaponDef.h"

#include <glm/glm.hpp>

#include <random>
#include <vector>

namespace weapons
{
    // Generates def.pelletsPerShot directions jittered inside a spreadDegrees half-angle
    // cone around aim_dir. Pure - independent of the raycast, so it is testable in
    // isolation with a seeded RNG for determinism.
    [[nodiscard]] std::vector<glm::vec3> pellet_directions(
        const glm::vec3& aim_dir, const WeaponDef& def, std::mt19937& rng);
}
