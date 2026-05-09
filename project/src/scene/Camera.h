#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
    Camera(glm::vec3 pos, float yaw = -90.0f, float pitch = 0.0f);

    [[nodiscard]] glm::mat4 getViewMatrix() const;

    void processMouseMotion(float xrel, float yrel, float sensitivity = 0.1f);

    void setPosition(glm::vec3 pos) { m_pos = pos; }
    [[nodiscard]] glm::vec3 position() const { return m_pos; }
    [[nodiscard]] glm::vec3 front() const { return m_front; }

private:
    glm::vec3 m_pos;
    glm::vec3 m_front;
    glm::vec3 m_right;
    glm::vec3 m_up;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    void updateVectors();
};
