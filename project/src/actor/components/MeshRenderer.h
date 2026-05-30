#pragma once

#include <glm/glm.hpp>

class Mesh;
class Shader;

class MeshRenderer
{
public:
    MeshRenderer(const Mesh* mesh, glm::vec3 color = glm::vec3(0.6f, 0.7f, 0.9f));

    void draw(Shader& shader, const glm::mat4& model) const;

    const Mesh* mesh      = nullptr;
    glm::vec3   color     = glm::vec3(0.6f, 0.7f, 0.9f);
    // Original color captured at construction; used as the base for damage tinting.
    glm::vec3   baseColor = glm::vec3(0.6f, 0.7f, 0.9f);
};
