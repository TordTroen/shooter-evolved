#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    Camera(glm::vec3 pos, float yaw = -90.0f, float pitch = 0.0f);

    [[nodiscard]] glm::mat4 getViewMatrix() const;

    void processMouseMotion(float xrel, float yrel, float sensitivity = 0.1f);
    void processKeyboard(const bool* keys, float deltaTime);

    [[nodiscard]] glm::vec3 position() const { return m_pos; }

    float moveSpeed = 5.0f;

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
