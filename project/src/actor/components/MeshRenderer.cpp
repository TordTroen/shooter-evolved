#include "MeshRenderer.h"

#include "rendering/Mesh.h"
#include "rendering/Shader.h"

MeshRenderer::MeshRenderer(const Mesh* mesh, glm::vec3 color)
    : mesh(mesh), color(color), baseColor(color)
{
}

void MeshRenderer::draw(Shader& shader, const glm::mat4& model) const
{
    shader.setMat4("model", model);
    shader.setVec3("uColor", color);
    mesh->draw();
}
