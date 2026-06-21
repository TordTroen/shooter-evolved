#include <catch2/catch_test_macros.hpp>

#include "rendering/DecalRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

static constexpr float kEps = 1e-4f;

static bool vec3_near(const glm::vec3& a, const glm::vec3& b, float eps = kEps)
{
    return std::abs(a.x - b.x) < eps
        && std::abs(a.y - b.y) < eps
        && std::abs(a.z - b.z) < eps;
}

// ---- Ring-buffer cap test ----
// DecalRenderer stores references to Mesh and Texture, but add() and active_count()
// never dereference them — only render() does. We bind references to zeroed storage
// so we can exercise the ring-buffer without a GL context.
namespace
{
    // Zeroed buffers large enough for any Mesh/Texture vtable + fields.
    alignas(64) static char s_mesh_buf[256] = {};
    alignas(64) static char s_tex_buf[256]  = {};
}

static DecalRenderer make_test_renderer()
{
    // NOLINT: intentional stub — render() is never called in these tests.
    return DecalRenderer(
        *reinterpret_cast<Mesh*>(s_mesh_buf),    // NOLINT
        *reinterpret_cast<Texture*>(s_tex_buf)); // NOLINT
}

TEST_CASE("DecalRenderer ring-buffer caps at MAX_DECALS")
{
    DecalRenderer dr = make_test_renderer();

    const glm::vec3 pos(0.0f, 0.0f, 0.0f);
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < DecalRenderer::MAX_DECALS + 10; ++i)
    {
        dr.add(pos, normal);
    }

    REQUIRE(dr.active_count() == DecalRenderer::MAX_DECALS);
}

// ---- Orientation test ----
TEST_CASE("build_model_matrix orients plane-local +Y to surface normal")
{
    const std::array<glm::vec3, 4> normals = {{
        glm::vec3(0.0f,  1.0f,  0.0f),   // floor
        glm::vec3(0.0f,  0.0f,  1.0f),   // front wall
        glm::vec3(1.0f,  0.0f,  0.0f),   // side wall
        glm::normalize(glm::vec3(0.3f, 0.7f, 0.5f)),  // oblique
    }};

    const glm::vec3 pos(1.0f, 2.0f, 3.0f);

    for (const auto& n : normals)
    {
        const glm::mat4 m = DecalRenderer::build_model_matrix(pos, n);

        // Column 1 of the matrix is the scaled +Y axis.
        // Normalising it should give back the original normal.
        const glm::vec3 y_col(m[1]);
        const float     y_len = glm::length(y_col);
        REQUIRE(y_len > 0.0f);
        const glm::vec3 mapped_y = y_col / y_len;
        REQUIRE(vec3_near(mapped_y, n));

        // Translation should be pos + n * SURFACE_OFFSET.
        const glm::vec3 expected_pos = pos + n * DecalRenderer::SURFACE_OFFSET;
        const glm::vec3 actual_pos(m[3]);
        REQUIRE(vec3_near(actual_pos, expected_pos));
    }
}

// ---- Degenerate normal test ----
TEST_CASE("DecalRenderer add with zero normal adds no decal")
{
    DecalRenderer dr = make_test_renderer();

    dr.add(glm::vec3(0.0f), glm::vec3(0.0f));

    REQUIRE(dr.active_count() == 0);
}

// ---- clear() test ----
TEST_CASE("DecalRenderer clear resets all decals")
{
    DecalRenderer dr = make_test_renderer();

    dr.add(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    dr.add(glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    REQUIRE(dr.active_count() == 2);

    dr.clear();
    REQUIRE(dr.active_count() == 0);
}
