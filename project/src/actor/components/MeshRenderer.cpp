#include "MeshRenderer.h"

#include "rendering/Mesh.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

namespace
{
    constexpr GLuint kBaseColorTextureUnit = 0;
}

MeshRenderer::MeshRenderer(const Mesh* mesh, glm::vec3 color)
    : mesh(mesh), color(color), baseColor(color)
{
}

void MeshRenderer::draw(Shader& shader, const glm::mat4& model) const
{
    shader.setMat4("model", model);
    shader.setVec3("uColor", color);

    if (texture)
    {
        texture->bind(kBaseColorTextureUnit);
        shader.setInt("uBaseColor",    static_cast<int>(kBaseColorTextureUnit));
        shader.setInt("uHasBaseColor", 1);
    }
    else
    {
        shader.setInt("uHasBaseColor", 0);
    }

    mesh->draw();
}
