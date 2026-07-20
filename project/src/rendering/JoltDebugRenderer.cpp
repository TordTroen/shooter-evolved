#include "JoltDebugRenderer.h"
#include "Shader.h"

#include <cstddef>
#include <glm/glm.hpp>

JoltDebugRenderer::JoltDebugRenderer()
{
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(LineVertex));

    // position - location 0 (matches basic.vert's layout; locations 1/2 (normal/uv) are
    // left disabled - flush() sets uEmissive so the shader never reads vNormal).
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(LineVertex, x));
    glVertexArrayAttribBinding(m_vao, 0, 0);

    // color - location 3
    glEnableVertexArrayAttrib(m_vao, 3);
    glVertexArrayAttribFormat(m_vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(LineVertex, r));
    glVertexArrayAttribBinding(m_vao, 3, 0);
}

JoltDebugRenderer::~JoltDebugRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    const float r = static_cast<float>(inColor.r) / 255.0f;
    const float g = static_cast<float>(inColor.g) / 255.0f;
    const float b = static_cast<float>(inColor.b) / 255.0f;

    m_lines.push_back({ static_cast<float>(inFrom.GetX()), static_cast<float>(inFrom.GetY()),
                         static_cast<float>(inFrom.GetZ()), r, g, b });
    m_lines.push_back({ static_cast<float>(inTo.GetX()), static_cast<float>(inTo.GetY()),
                         static_cast<float>(inTo.GetZ()), r, g, b });
}

void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg /*inPosition*/, const std::string_view& /*inString*/,
                                   JPH::ColorArg /*inColor*/, float /*inHeight*/)
{
    // Not needed for collision-shape visualization; text labels are a no-op.
}

void JoltDebugRenderer::flush(Shader& shader)
{
    if (m_lines.empty()) { return; }

    glNamedBufferData(m_vbo,
        static_cast<GLsizeiptr>(m_lines.size() * sizeof(LineVertex)),
        m_lines.data(), GL_DYNAMIC_DRAW);

    shader.setMat4("model", glm::mat4(1.0f));
    shader.setVec3("uColor", glm::vec3(1.0f));
    shader.setInt("uHasBaseColor", 0);
    // Skip the normal-based lighting path (locations 1/2 are disabled on this VAO, so
    // vNormal would be zero and normalize() it produces NaN) - emissive just outputs
    // uColor * vColor directly.
    shader.setInt("uEmissive", 1);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lines.size()));

    // Restore: MeshRenderer::draw() never sets uEmissive itself (relies on the GL
    // default of 0 immediately after linking), so leaving it at 1 would silently break
    // lighting for every actor/weapon drawn next frame.
    shader.setInt("uEmissive", 0);

    m_lines.clear();
}
