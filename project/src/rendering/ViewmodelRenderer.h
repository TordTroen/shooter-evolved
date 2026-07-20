#pragma once
#include "../weapons/WeaponDef.h"
#include <glm/glm.hpp>

class Camera;
class MeshRenderer;
class Shader;

// Stateless: the mesh + transform (WeaponDef's rightOffset/downOffset/forwardOffset/scale/
// axisFixDegrees) are passed per call rather than fixed at construction, so a single
// instance can render whichever weapon is currently equipped (MultipleWeapons.md).
class ViewmodelRenderer
{
public:
    // Clears GL_DEPTH_BUFFER_BIT then draws meshRenderer in camera-relative space using
    // def's viewmodel transform. axisFixDegrees: yaw applied around Y to align the model's
    // local forward axis with camera-forward (e.g. BasicGun.glb needs -90).
    void render(Shader& shader, const Camera& camera, MeshRenderer& meshRenderer,
                const weapons::WeaponDef& def) const;

    // World-space position at forwardOffset along the camera, using def's right/down
    // offsets - used to place the muzzle flash.
    [[nodiscard]] glm::vec3 muzzleWorldPos(const Camera& camera, const weapons::WeaponDef& def,
                                            float forwardOffset) const;
};
