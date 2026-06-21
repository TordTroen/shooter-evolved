#pragma once
#include <cstdint>
#include <vector>

namespace ProceduralTextures
{
    // Generates an RGBA muzzle-flash texture: radial falloff with angular spikes,
    // white-hot core fading to orange. Returns size*size*4 bytes.
    std::vector<uint8_t> muzzleFlashRGBA(int size);

    // Generates an RGBA bullet-hole texture: black disc with soft edge alpha falloff.
    // Returns size*size*4 bytes.
    std::vector<uint8_t> bulletHoleRGBA(int size);
}
