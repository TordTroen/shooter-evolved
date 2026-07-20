#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include <glad/glad.h>

#include <string_view>
#include <vector>

class Shader;

// Draws Jolt physics collision shapes as wireframe lines through the existing basic
// shader/GL pipeline (dev tool - "visualize the Jolt collision boxes"). Inherits from
// DebugRendererSimple rather than DebugRenderer directly: it already implements
// CreateTriangleBatch/DrawGeometry (by tessellating into DrawTriangle calls) and
// DrawTriangle (as three DrawLine calls), so wireframes only need DrawLine + DrawText3D.
class JoltDebugRenderer final : public JPH::DebugRendererSimple
{
public:
    JoltDebugRenderer();
    ~JoltDebugRenderer() override;

    JoltDebugRenderer(const JoltDebugRenderer&)            = delete;
    JoltDebugRenderer& operator=(const JoltDebugRenderer&) = delete;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString,
                     JPH::ColorArg inColor, float inHeight) override;

    // Uploads every line buffered since the last flush and draws it (view/projection
    // already set by the caller - PlayingState::render()), then clears the buffer so
    // next frame's DrawLine calls start fresh.
    void flush(Shader& shader);

private:
    struct LineVertex { float x, y, z, r, g, b; };

    GLuint                   m_vao = 0;
    GLuint                   m_vbo = 0;
    std::vector<LineVertex>  m_lines;
};
