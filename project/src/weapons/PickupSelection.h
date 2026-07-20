#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace weapons
{
    constexpr float kPickupRange = 2.5f; // metres; server-authoritative distance check

    struct PickupCandidate
    {
        uint32_t  netId;
        glm::vec3 position;
    };

    // Nearest candidate strictly within range of playerPos; nullopt if none qualify.
    // Ties are broken by vector order (first at the minimal distance wins), independent
    // of physics, for deterministic server behaviour.
    std::optional<PickupCandidate> nearest_item_in_range(const glm::vec3& playerPos,
                                                          const std::vector<PickupCandidate>& items,
                                                          float range = kPickupRange);
}
