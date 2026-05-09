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

void Camera::processKeyboard(const bool* keys, float deltaTime) {
    const float speed = moveSpeed * deltaTime;
    if (keys[SDL_SCANCODE_W]) m_pos += m_front * speed;
    if (keys[SDL_SCANCODE_S]) m_pos -= m_front * speed;
    if (keys[SDL_SCANCODE_A]) m_pos -= m_right * speed;
    if (keys[SDL_SCANCODE_D]) m_pos += m_right * speed;
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
