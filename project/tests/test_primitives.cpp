#include "rendering/Primitives.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

TEST_CASE("Primitives::planeVertices has 4 vertices")
{
    REQUIRE(Primitives::planeVertices().size() == 4);
}

TEST_CASE("Primitives::planeIndices has 6 indices")
{
    REQUIRE(Primitives::planeIndices().size() == 6);
}

TEST_CASE("Primitives::boxVertices has 24 vertices")
{
    REQUIRE(Primitives::boxVertices().size() == 24);
}

TEST_CASE("Primitives::boxIndices has 36 indices")
{
    REQUIRE(Primitives::boxIndices().size() == 36);
}

TEST_CASE("Primitives plane indices are all in range")
{
    const auto verts   = Primitives::planeVertices();
    const auto indices = Primitives::planeIndices();
    for (uint32_t idx : indices)
        REQUIRE(idx < static_cast<uint32_t>(verts.size()));
}

TEST_CASE("Primitives box indices are all in range")
{
    const auto verts   = Primitives::boxVertices();
    const auto indices = Primitives::boxIndices();
    for (uint32_t idx : indices)
        REQUIRE(idx < static_cast<uint32_t>(verts.size()));
}

TEST_CASE("Primitives plane normals are unit length")
{
    for (const Vertex& v : Primitives::planeVertices())
    {
        const float len = std::sqrt(v.normal.x * v.normal.x +
                                    v.normal.y * v.normal.y +
                                    v.normal.z * v.normal.z);
        REQUIRE_THAT(len, Catch::Matchers::WithinAbs(1.0f, 1e-5f));
    }
}

TEST_CASE("Primitives box normals are unit length")
{
    for (const Vertex& v : Primitives::boxVertices())
    {
        const float len = std::sqrt(v.normal.x * v.normal.x +
                                    v.normal.y * v.normal.y +
                                    v.normal.z * v.normal.z);
        REQUIRE_THAT(len, Catch::Matchers::WithinAbs(1.0f, 1e-5f));
    }
}
