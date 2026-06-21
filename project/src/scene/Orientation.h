#pragma once
#include <glm/glm.hpp>

namespace orientation
{

struct Basis
{
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
};

// Single source of truth for yaw/pitch → directional basis conversion.
// Mirrors Camera::updateVectors exactly; both Camera and RemotePlayerRenderer call this.
Basis basis_from_yaw_pitch(float yaw_deg, float pitch_deg);

} // namespace orientation
