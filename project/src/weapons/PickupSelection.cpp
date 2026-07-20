#include "PickupSelection.h"

namespace weapons
{
    std::optional<PickupCandidate> nearest_item_in_range(const glm::vec3& playerPos,
                                                          const std::vector<PickupCandidate>& items,
                                                          float range)
    {
        std::optional<PickupCandidate> best;
        float bestDistSq = range * range;

        for (const auto& item : items)
        {
            const glm::vec3 delta  = item.position - playerPos;
            const float     distSq = glm::dot(delta, delta);
            if (distSq < bestDistSq)
            {
                bestDistSq = distSq;
                best       = item;
            }
        }
        return best;
    }
}
