#include "DecalRenderer.h"

#include "rendering/Mesh.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include <glad/glad.h>

void DecalRenderer::render(Shader& shader) const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_texture.bind(0);
    shader.setInt ("uBaseColor",    0);
    shader.setInt ("uHasBaseColor", 1);
    shader.setInt ("uEmissive",     1);
    shader.setVec3("uColor",        glm::vec3(1.0f));

    for (const auto& d : m_decals)
    {
        if (!d.is_active) { continue; }
        shader.setMat4("model", d.model);
        m_planeMesh.draw();
    }

    shader.setInt("uEmissive", 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
