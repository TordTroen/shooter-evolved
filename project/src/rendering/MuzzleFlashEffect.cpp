#include "MuzzleFlashEffect.h"

#include "rendering/Mesh.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

MuzzleFlashEffect::MuzzleFlashEffect(const Mesh& planeMesh, const Texture& flashTexture)
    : m_planeMesh(planeMesh)
    , m_texture(flashTexture)
{
}

void MuzzleFlashEffect::trigger()
{
    m_timer = kDuration;
}

void MuzzleFlashEffect::render(Shader& shader,
                               const glm::vec3& muzzlePos,
                               const glm::vec3& camRight,
                               const glm::vec3& camUp,
                               const glm::vec3& negCamFront,
                               float deltaTime)
{
    if (m_timer <= 0.0f)
        return;

    const float t     = m_timer / kDuration;
    const float fade  = std::clamp(t, 0.0f, 1.0f);
    const float scale = kFlashSize * (0.8f + 0.4f * fade);

    // Billboard: plane-Y maps to -camFront so the quad faces the camera.
    glm::mat4 billboard(1.0f);
    billboard[0] = glm::vec4(camRight,    0.0f);
    billboard[1] = glm::vec4(negCamFront, 0.0f);
    billboard[2] = glm::vec4(camUp,       0.0f);

    const glm::mat4 flashMat =
        glm::translate(glm::mat4(1.0f), muzzlePos) *
        billboard *
        glm::scale(glm::mat4(1.0f), glm::vec3(scale));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
    glDepthMask(GL_FALSE);

    m_texture.bind(0);
    shader.setInt ("uBaseColor",    0);
    shader.setInt ("uHasBaseColor", 1);
    shader.setInt ("uEmissive",     1);
    shader.setVec3("uColor",        glm::vec3(fade));
    shader.setMat4("model",         flashMat);
    m_planeMesh.draw();
    shader.setInt ("uEmissive",     0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    m_timer -= deltaTime;
}
