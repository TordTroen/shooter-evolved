#pragma once
#include <glm/glm.hpp>

class MeshRenderer;
class Shader;
struct PlayerState;

class RemotePlayerRenderer
{
public:
    // bodyMesh: box mesh used to draw the body (not owned).
    // gunMR: gun MeshRenderer (not owned).
    // rightOffset/downOffset/forwardOffset: shoulder anchor relative to player position,
    //   matching the ViewmodelRenderer's offsets so the gun looks identical in world space.
    // scale/axisFixDegrees: same model-space corrections as ViewmodelRenderer.
    RemotePlayerRenderer(MeshRenderer& body_mesh_renderer,
                         MeshRenderer& gun_mesh_renderer,
                         float right_offset    =  0.25f,
                         float down_offset     = 1.20f,
                         float forward_offset  =  0.45f,
                         float scale           =  0.25f,
                         float axis_fix_degrees = -90.0f);

    void render(Shader& shader, const PlayerState& ps) const;

private:
    MeshRenderer& m_bodyMR;
    MeshRenderer& m_gunMR;
    float m_rightOffset;
    float m_downOffset;
    float m_forwardOffset;
    float m_scale;
    float m_axisFixDegrees;
};
