#include "Mesh.h"
#include <cstddef>

Mesh::Mesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices)
    : m_indexCount(static_cast<GLsizei>(indices.size()))
{
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);

    glNamedBufferStorage(m_vbo,
        static_cast<GLsizeiptr>(vertices.size_bytes()),
        vertices.data(), 0);
    glNamedBufferStorage(m_ebo,
        static_cast<GLsizeiptr>(indices.size_bytes()),
        indices.data(), 0);

    glVertexArrayVertexBuffer (m_vao, 0, m_vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(m_vao, m_ebo);

    // position - location 0
    glEnableVertexArrayAttrib (m_vao, 0);
    glVertexArrayAttribFormat (m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(m_vao, 0, 0);

    // normal - location 1
    glEnableVertexArrayAttrib (m_vao, 1);
    glVertexArrayAttribFormat (m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(m_vao, 1, 0);

    // uv - location 2
    glEnableVertexArrayAttrib (m_vao, 2);
    glVertexArrayAttribFormat (m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribBinding(m_vao, 2, 0);

    // color - location 3
    glEnableVertexArrayAttrib (m_vao, 3);
    glVertexArrayAttribFormat (m_vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
    glVertexArrayAttribBinding(m_vao, 3, 0);
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &m_ebo);
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void Mesh::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}
