#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cmath>

class Mesh;
class Shader;
class Texture;

class DecalRenderer
{
public:
    static constexpr int   MAX_DECALS     = 128;
    static constexpr float DECAL_SIZE     = 0.05f;   // metres (quad edge length)
    static constexpr float SURFACE_OFFSET = 0.01f;   // push off surface to avoid z-fighting

    DecalRenderer(const Mesh& planeMesh, const Texture& decalTexture);

    // Place a decal flush against a surface. `normal` is the world-space surface normal.
    // Skips degenerate (zero) normals.
    void add(const glm::vec3& position, const glm::vec3& normal);

    // Draw all active decals. Does not modify GL state permanently.
    void render(Shader& shader) const;

    void clear();

    [[nodiscard]] int active_count() const;

    // Builds the model matrix for a decal at `position` oriented to `normal`.
    // Returns identity if normal is degenerate (length < 1e-4). Pure GLM, no GL.
    [[nodiscard]] static glm::mat4 build_model_matrix(const glm::vec3& position,
                                                       const glm::vec3& normal);

private:
    struct Decal
    {
        glm::mat4 model{ 1.0f };
        bool      is_active = false;
    };

    const Mesh&    m_planeMesh;
    const Texture& m_texture;
    std::array<Decal, MAX_DECALS> m_decals{};
    int            m_next = 0;  // ring-buffer write cursor
};

// ---- inline implementations (pure GLM, no GL dependency) ----

inline DecalRenderer::DecalRenderer(const Mesh& planeMesh, const Texture& decalTexture)
    : m_planeMesh(planeMesh)
    , m_texture(decalTexture)
{}

inline glm::mat4 DecalRenderer::build_model_matrix(const glm::vec3& position,
                                                    const glm::vec3& normal)
{
    if (glm::length(normal) < 1e-4f)
    {
        return glm::mat4(1.0f);
    }

    const glm::vec3 n = glm::normalize(normal);

    // Choose a reference axis not parallel to n.
    const glm::vec3 ref = (std::abs(n.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                    : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 tangent   = glm::normalize(glm::cross(ref, n));
    const glm::vec3 bitangent = glm::cross(n, tangent);

    // Columns: X=tangent, Y=normal (local +Y aligns to surface), Z=bitangent, W=translation.
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(tangent   * DECAL_SIZE, 0.0f);
    m[1] = glm::vec4(n         * DECAL_SIZE, 0.0f);
    m[2] = glm::vec4(bitangent * DECAL_SIZE, 0.0f);
    m[3] = glm::vec4(position  + n * SURFACE_OFFSET, 1.0f);
    return m;
}

inline void DecalRenderer::add(const glm::vec3& position, const glm::vec3& normal)
{
    if (glm::length(normal) < 1e-4f)
    {
        return;
    }
    m_decals[m_next] = { build_model_matrix(position, normal), true };
    m_next = (m_next + 1) % MAX_DECALS;
}

inline void DecalRenderer::clear()
{
    m_decals = {};
    m_next   = 0;
}

inline int DecalRenderer::active_count() const
{
    int count = 0;
    for (const auto& d : m_decals)
    {
        if (d.is_active) { ++count; }
    }
    return count;
}
