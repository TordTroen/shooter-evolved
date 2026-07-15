#pragma once
#include <glm/glm.hpp>

class Camera;
class MeshRenderer;
class Shader;

class ViewmodelRenderer
{
public:
    // axisFixDegrees: yaw applied around Y to align the model's local forward
    // axis with camera-forward. BasicGun.glb needs -90.
    explicit ViewmodelRenderer(MeshRenderer& meshRenderer,
                               float rightOffset    =  0.25f,
                               float downOffset     = -0.20f,
                               float forwardOffset  =  0.45f,
                               float scale          =  0.25f,
                               float axisFixDegrees = -90.0f);

    // Clears GL_DEPTH_BUFFER_BIT then draws the mesh in camera-relative space.
    void render(Shader& shader, const Camera& camera) const;

    // World-space position at forwardOffset along the camera, using the same
    // right/down offsets as the viewmodel - used to place the muzzle flash.
    [[nodiscard]] glm::vec3 muzzleWorldPos(const Camera& camera, float forwardOffset) const;

private:
    MeshRenderer& m_meshRenderer;
    float m_rightOffset;
    float m_downOffset;
    float m_forwardOffset;
    float m_scale;
    float m_axisFixDegrees;
};
