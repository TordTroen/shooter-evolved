#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class JoltDebugRenderer;

// Higher-level debug-drawing helpers for game objects that have no client-side physics
// body of their own (e.g. replicated weapon items - see MultipleWeapons.md), so they
// can't rely on JoltDebugRenderer::flush()'s normal PhysicsSystem::DrawBodies() path.
// Not Jolt-specific logic itself - just shape math feeding JoltDebugRenderer's line buffer.
namespace DebugDraw
{
    void wireBox(JoltDebugRenderer& renderer, const glm::vec3& center, const glm::quat& rotation,
                const glm::vec3& halfExtents, JPH::ColorArg color);
}
