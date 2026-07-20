#pragma once
#include "../weapons/WeaponDef.h"
#include <glm/glm.hpp>

class MeshRenderer;
class Shader;
struct PlayerState;

class RemotePlayerRenderer
{
public:
    // bodyMesh: box mesh used to draw the body (not owned).
    // rightOffset/downOffset/forwardOffset: shoulder anchor relative to player position -
    // weapon-agnostic world-space geometry (unlike ViewmodelRenderer's per-weapon camera-
    // relative offsets), so these stay fixed constructor defaults, not sourced from WeaponDef.
    explicit RemotePlayerRenderer(MeshRenderer& body_mesh_renderer,
                                  float right_offset    =  0.25f,
                                  float down_offset     = 1.20f,
                                  float forward_offset  =  0.45f);

    // gun_mesh_renderer/def: the equipped weapon's cached mesh and definition (WeaponModelCache
    // + registry lookup keyed by PlayerState::equippedWeapon) - MultipleWeapons.md. def's
    // scale/axisFixDegrees are the same model-space corrections ViewmodelRenderer uses.
    void render(Shader& shader, const PlayerState& ps, MeshRenderer& gun_mesh_renderer,
                const weapons::WeaponDef& def) const;
    glm::vec3 muzzle_world_pos(const PlayerState& ps, float forward_offset = 0.2f) const;

private:
    MeshRenderer& m_bodyMR;
    float m_rightOffset;
    float m_downOffset;
    float m_forwardOffset;
};
