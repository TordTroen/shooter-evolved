#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "scene/Orientation.h"

#include <glm/glm.hpp>
#include <cmath>

static constexpr float kEps = 1e-5f;

static bool vec3_near(glm::vec3 a, glm::vec3 b, float eps = kEps)
{
    return std::abs(a.x - b.x) < eps
        && std::abs(a.y - b.y) < eps
        && std::abs(a.z - b.z) < eps;
}

// Default yaw=-90, pitch=0 → front should point down -Z (same as Camera default).
TEST_CASE("basis_from_yaw_pitch: default yaw=-90 pitch=0 points down -Z")
{
    const auto b = orientation::basis_from_yaw_pitch(-90.0f, 0.0f);
    REQUIRE(vec3_near(b.front, glm::vec3(0.0f, 0.0f, -1.0f)));
}

// yaw=0, pitch=0 → front points down +X.
TEST_CASE("basis_from_yaw_pitch: yaw=0 pitch=0 points down +X")
{
    const auto b = orientation::basis_from_yaw_pitch(0.0f, 0.0f);
    REQUIRE(vec3_near(b.front, glm::vec3(1.0f, 0.0f, 0.0f)));
}

// pitch=89 → mostly up, very little horizontal.
TEST_CASE("basis_from_yaw_pitch: pitch=89 tilts strongly upward")
{
    const auto b = orientation::basis_from_yaw_pitch(-90.0f, 89.0f);
    REQUIRE(b.front.y > 0.99f);
    REQUIRE(std::abs(b.front.x) < 0.02f);
}

// Right is cross(front, worldUp) normalized - must be perpendicular to front.
TEST_CASE("basis_from_yaw_pitch: basis vectors are orthogonal")
{
    const auto b = orientation::basis_from_yaw_pitch(45.0f, 30.0f);
    REQUIRE(std::abs(glm::dot(b.front, b.right)) < kEps);
    REQUIRE(std::abs(glm::dot(b.front, b.up))    < kEps);
    REQUIRE(std::abs(glm::dot(b.right, b.up))    < kEps);
}
