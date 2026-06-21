#include "Camera.h"
#include "Orientation.h"

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

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_pos, m_pos + m_front, m_up);
}

void Camera::processMouseMotion(float xrel, float yrel, float sensitivity)
{
    m_yaw   += xrel * sensitivity;
    m_pitch -= yrel * sensitivity;  // yrel positive = moving down on screen
    m_pitch  = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

void Camera::updateVectors()
{
    const orientation::Basis b = orientation::basis_from_yaw_pitch(m_yaw, m_pitch);
    m_front = b.front;
    m_right = b.right;
    m_up    = b.up;
}
