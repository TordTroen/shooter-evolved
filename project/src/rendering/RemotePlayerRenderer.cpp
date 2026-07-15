#include "RemotePlayerRenderer.h"

#include "actor/components/MeshRenderer.h"
#include "net/PlayerState.h"
#include "rendering/Shader.h"
#include "scene/Orientation.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

RemotePlayerRenderer::RemotePlayerRenderer(MeshRenderer& body_mesh_renderer,
                                           MeshRenderer& gun_mesh_renderer,
                                           float right_offset,
                                           float down_offset,
                                           float forward_offset,
                                           float scale,
                                           float axis_fix_degrees)
    : m_bodyMR(body_mesh_renderer)
    , m_gunMR(gun_mesh_renderer)
    , m_rightOffset(right_offset)
    , m_downOffset(down_offset)
    , m_forwardOffset(forward_offset)
    , m_scale(scale)
    , m_axisFixDegrees(axis_fix_degrees)
{
}

void RemotePlayerRenderer::render(Shader& shader, const PlayerState& ps) const
{
    // Body: yaw only - keeps the box upright while facing the player's look direction.
    // ps.position is feet-level; the unit-cube mesh is centered at its origin, so we
    // shift up by half the body height (0.9 m) to align the bottom with the feet.
    const glm::vec3 body_pos = ps.position + glm::vec3(0.0f, 0.9f, 0.0f);
    const glm::mat4 body_model =
        glm::translate(glm::mat4(1.0f), body_pos) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-ps.yaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 1.8f, 0.6f));
    m_bodyMR.draw(shader, body_model);

    // Gun: full yaw + pitch, anchored at the player's shoulder.
    const orientation::Basis b = orientation::basis_from_yaw_pitch(ps.yaw, ps.pitch);

    const glm::vec3 gun_pos = ps.position
        + b.right * m_rightOffset
        + b.up    * m_downOffset
        + b.front * m_forwardOffset;

    // glTF convention: model's -Z maps to world-forward (b.front).
    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(b.right,  0.0f);
    rot[1] = glm::vec4(b.up,     0.0f);
    rot[2] = glm::vec4(-b.front, 0.0f);

    const glm::mat4 axis_fix =
        glm::rotate(glm::mat4(1.0f), glm::radians(m_axisFixDegrees), glm::vec3(0.0f, 1.0f, 0.0f));

    const glm::mat4 gun_model =
        glm::translate(glm::mat4(1.0f), gun_pos) *
        rot *
        axis_fix *
        glm::scale(glm::mat4(1.0f), glm::vec3(m_scale));

    // World-space gun depth-tests normally - no glClear(GL_DEPTH_BUFFER_BIT) here.
    m_gunMR.draw(shader, gun_model);
}

glm::vec3 RemotePlayerRenderer::muzzle_world_pos(const PlayerState& ps, float forward_offset) const
{
    const orientation::Basis b = orientation::basis_from_yaw_pitch(ps.yaw, ps.pitch);
    const glm::vec3 gun_pos = ps.position
        + b.right * m_rightOffset
        + b.up    * m_downOffset
        + b.front * m_forwardOffset;
    return gun_pos + b.front * forward_offset;
}
