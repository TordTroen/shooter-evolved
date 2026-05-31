#pragma once
#include <glm/glm.hpp>

class Mesh;
class Shader;
class Texture;

class MuzzleFlashEffect
{
public:
    MuzzleFlashEffect(const Mesh& planeMesh, const Texture& flashTexture);

    void trigger();

    // Renders the billboard if the effect is active. muzzlePos is the
    // world-space position of the barrel tip. camRight/camUp/negCamFront are
    // the camera basis vectors (negCamFront = -camera.front()).
    // Decrements the internal timer by deltaTime.
    void render(Shader& shader,
                const glm::vec3& muzzlePos,
                const glm::vec3& camRight,
                const glm::vec3& camUp,
                const glm::vec3& negCamFront,
                float deltaTime);

private:
    const Mesh&    m_planeMesh;
    const Texture& m_texture;
    float          m_timer = 0.0f;

    static constexpr float kDuration  = 0.2f;
    static constexpr float kFlashSize = 0.30f;
};
