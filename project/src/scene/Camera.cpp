#include "Camera.h"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <numbers>
#include <cmath>
#include <algorithm>

Camera::Camera(glm::vec3 pos, float yaw, float pitch)
    : m_pos(pos)
    , m_worldUp(0.0f, 1.0f, 0.0f)
    , m_yaw(yaw)
    , m_pitch(pitch)
{
    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_pos, m_pos + m_front, m_up);
}

void Camera::processMouseMotion(float xrel, float yrel, float sensitivity) {
    m_yaw   += xrel * sensitivity;
    m_pitch -= yrel * sensitivity;  // yrel positive = moving down on screen
    m_pitch  = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

glm::vec3 Camera::getWishVelocity(const bool* keys, float speed) const {
    // Flatten front onto the XZ plane so vertical look angle doesn't affect move speed.
    const glm::vec3 flatFront = glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    const glm::vec3 flatRight = glm::normalize(glm::vec3(m_right.x, 0.0f, m_right.z));

    glm::vec3 dir(0.0f);
    if (keys[SDL_SCANCODE_W]) dir += flatFront;
    if (keys[SDL_SCANCODE_S]) dir -= flatFront;
    if (keys[SDL_SCANCODE_A]) dir -= flatRight;
    if (keys[SDL_SCANCODE_D]) dir += flatRight;

    if (glm::length(dir) > 0.001f)
        dir = glm::normalize(dir) * speed;
    return dir;
}

void Camera::updateVectors() {
    const float yawRad   = glm::radians(m_yaw);
    const float pitchRad = glm::radians(m_pitch);
    m_front = glm::normalize(glm::vec3{
        std::cos(yawRad) * std::cos(pitchRad),
        std::sin(pitchRad),
        std::sin(yawRad) * std::cos(pitchRad)
    });
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}
