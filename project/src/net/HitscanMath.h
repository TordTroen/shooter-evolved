#pragma once

#include <glm/glm.hpp>
#include <cmath>

// Player capsule geometry - matches CharacterController.cpp kCapsule* constants.
static constexpr float kPlayerCapsuleHalfHeight = 0.7f;
static constexpr float kPlayerCapsuleRadius     = 0.3f;

// Compute the capsule sphere-center endpoints from the player's feet position.
// (Mirrors the RotatedTranslatedShape offset used in CharacterController.)
inline void player_capsule_endpoints(const glm::vec3& feet,
                                     glm::vec3& out_bottom,
                                     glm::vec3& out_top)
{
    out_bottom = feet + glm::vec3(0.0f, kPlayerCapsuleRadius, 0.0f);
    out_top    = feet + glm::vec3(0.0f, kPlayerCapsuleRadius + 2.0f * kPlayerCapsuleHalfHeight, 0.0f);
}

// Ray vs. capsule intersection.
// Returns the hit distance t >= 0.0f, or -1.0f on miss.
// ray_dir must be normalized. cap_a and cap_b are the sphere-center endpoints.
inline float ray_vs_capsule(
    const glm::vec3& ray_origin, const glm::vec3& ray_dir,
    const glm::vec3& cap_a,     const glm::vec3& cap_b,
    float            cap_radius)
{
    const glm::vec3 ba   = cap_b - cap_a;
    const glm::vec3 oa   = ray_origin - cap_a;
    const float baba     = glm::dot(ba, ba);
    const float bard     = glm::dot(ba, ray_dir);
    const float baoa     = glm::dot(ba, oa);
    const float rdoa     = glm::dot(ray_dir, oa);
    const float oaoa     = glm::dot(oa, oa);
    const float a        = baba - bard * bard;
    const float b        = baba * rdoa - baoa * bard;
    const float c        = baba * oaoa - baoa * baoa - cap_radius * cap_radius * baba;

    float result = -1.0f;

    // Cylinder body: skip if ray is parallel to the capsule axis.
    if (std::abs(a) > 1e-6f)
    {
        const float h = b * b - a * c;
        if (h >= 0.0f)
        {
            const float t = (-b - std::sqrt(h)) / a;
            if (t >= 0.0f)
            {
                const float y = baoa + t * bard;
                if (y > 0.0f && y < baba)
                    result = t;
            }
        }
    }

    // Sphere caps - try the nearer one if better than the body hit.
    auto try_cap = [&](const glm::vec3& oc)
    {
        const float bc = glm::dot(ray_dir, oc);
        const float cc = glm::dot(oc, oc) - cap_radius * cap_radius;
        const float hc = bc * bc - cc;
        if (hc >= 0.0f)
        {
            const float t = -bc - std::sqrt(hc);
            if (t >= 0.0f && (result < 0.0f || t < result))
                result = t;
        }
    };

    try_cap(oa);        // bottom cap: oc = ray_origin - cap_a
    try_cap(ray_origin - cap_b); // top cap: oc = ray_origin - cap_b

    return result;
}
