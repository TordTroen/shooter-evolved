#include "Orientation.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace orientation
{

Basis basis_from_yaw_pitch(float yaw_deg, float pitch_deg)
{
    const float yaw_rad   = glm::radians(yaw_deg);
    const float pitch_rad = glm::radians(pitch_deg);

    const glm::vec3 world_up(0.0f, 1.0f, 0.0f);

    Basis b;
    b.front = glm::normalize(glm::vec3{
        std::cos(yaw_rad) * std::cos(pitch_rad),
        std::sin(pitch_rad),
        std::sin(yaw_rad) * std::cos(pitch_rad)
    });
    b.right = glm::normalize(glm::cross(b.front, world_up));
    b.up    = glm::normalize(glm::cross(b.right, b.front));
    return b;
}

} // namespace orientation
