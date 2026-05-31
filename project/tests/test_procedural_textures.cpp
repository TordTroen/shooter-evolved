#include "rendering/ProceduralTextures.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ProceduralTextures::muzzleFlashRGBA size=32 produces correct byte count")
{
    const auto pixels = ProceduralTextures::muzzleFlashRGBA(32);
    REQUIRE(pixels.size() == 32 * 32 * 4);
}

TEST_CASE("ProceduralTextures::muzzleFlashRGBA centre pixel alpha is near 255")
{
    constexpr int size = 32;
    const auto pixels = ProceduralTextures::muzzleFlashRGBA(size);
    // Centre pixel: (size/2, size/2)
    const size_t cx = size / 2;
    const size_t cy = size / 2;
    const size_t i  = (cy * size + cx) * 4;
    // Alpha channel (index +3) should be near 255 at the hot core.
    REQUIRE(pixels[i + 3] > 200);
}

TEST_CASE("ProceduralTextures::muzzleFlashRGBA corner pixel alpha is 0")
{
    constexpr int size = 32;
    const auto pixels = ProceduralTextures::muzzleFlashRGBA(size);
    // Top-left corner pixel (0, 0) — farthest from centre, radial falloff → 0.
    const size_t i = 0;
    REQUIRE(pixels[i + 3] == 0);
}
