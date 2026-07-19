#include "Pellets.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace weapons
{
    std::vector<glm::vec3> pellet_directions(
        const glm::vec3& aim_dir, const WeaponDef& def, std::mt19937& rng)
    {
        std::vector<glm::vec3> result;
        result.reserve(static_cast<size_t>(std::max(def.pelletsPerShot, 0)));

        // Zero spread: every pellet is exactly the aim direction, no RNG involved -
        // reduces bit-for-bit to today's single-pellet behaviour.
        if (def.spreadDegrees <= 0.0f)
        {
            for (int i = 0; i < def.pelletsPerShot; ++i)
            {
                result.push_back(aim_dir);
            }
            return result;
        }

        // Build an orthonormal basis perpendicular to aim_dir to jitter within.
        glm::vec3 tangent = glm::abs(aim_dir.y) < 0.99f
            ? glm::normalize(glm::cross(aim_dir, glm::vec3(0.0f, 1.0f, 0.0f)))
            : glm::normalize(glm::cross(aim_dir, glm::vec3(1.0f, 0.0f, 0.0f)));
        const glm::vec3 bitangent = glm::normalize(glm::cross(aim_dir, tangent));

        const float spread_rad = glm::radians(def.spreadDegrees);
        std::uniform_real_distribution<float> theta_dist(0.0f, spread_rad);
        std::uniform_real_distribution<float> phi_dist(0.0f, glm::two_pi<float>());

        for (int i = 0; i < def.pelletsPerShot; ++i)
        {
            const float theta = theta_dist(rng);
            const float phi   = phi_dist(rng);

            // Offset stays perpendicular to aim_dir, so the angle between aim_dir and the
            // resulting direction is exactly theta (<= spreadDegrees).
            const glm::vec3 offset = std::tan(theta) *
                (std::cos(phi) * tangent + std::sin(phi) * bitangent);

            result.push_back(glm::normalize(aim_dir + offset));
        }

        return result;
    }
}
