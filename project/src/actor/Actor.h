#pragma once
#include <glm/glm.hpp>

class Mesh;
class Shader;

class Actor
{
public:
    Actor(const Mesh* mesh, glm::mat4 model = glm::mat4(1.0f));
    virtual ~Actor() = default;

    virtual void update(float /*dt*/) {}
    void draw(Shader& shader) const;

    const Mesh* mesh  = nullptr;
    glm::mat4   model = glm::mat4(1.0f);
    glm::vec3   color = glm::vec3(0.6f, 0.7f, 0.9f);
};
