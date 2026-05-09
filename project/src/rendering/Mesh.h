#pragma once
#include <glad/glad.h>
#include <span>
#include <cstdint>
#include "Vertex.h"

class Mesh
{
public:
    Mesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

private:
    GLuint  m_vao        = 0;
    GLuint  m_vbo        = 0;
    GLuint  m_ebo        = 0;
    GLsizei m_indexCount = 0;
};
