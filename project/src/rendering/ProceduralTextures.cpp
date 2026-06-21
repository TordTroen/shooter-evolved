#include "ProceduralTextures.h"

#include <algorithm>
#include <cmath>

namespace ProceduralTextures
{
    std::vector<uint8_t> muzzleFlashRGBA(int size)
    {
        std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);
        const float center = (size - 1) * 0.5f;
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const float dx = (x - center) / center;
                const float dy = (y - center) / center;
                const float d  = std::sqrt(dx * dx + dy * dy);

                const float angle  = std::atan2(dy, dx);
                const float spikes = 0.85f + 0.15f * std::cos(angle * 5.0f);
                const float radial = std::clamp(1.0f - d / spikes, 0.0f, 1.0f);
                const float core   = std::pow(radial, 2.2f);

                const float r = std::clamp(core * 1.6f, 0.0f, 1.0f);
                const float g = std::clamp(core * 1.1f, 0.0f, 1.0f);
                const float b = std::clamp(core * 0.5f, 0.0f, 1.0f);
                const float a = core;

                const size_t i = (static_cast<size_t>(y) * size + x) * 4;
                pixels[i + 0] = static_cast<uint8_t>(r * 255.0f);
                pixels[i + 1] = static_cast<uint8_t>(g * 255.0f);
                pixels[i + 2] = static_cast<uint8_t>(b * 255.0f);
                pixels[i + 3] = static_cast<uint8_t>(a * 255.0f);
            }
        }
        return pixels;
    }

    std::vector<uint8_t> bulletHoleRGBA(int size)
    {
        std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);
        const float center = (size - 1) * 0.5f;
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const float dx = (x - center) / center;
                const float dy = (y - center) / center;
                const float d  = std::sqrt(dx * dx + dy * dy);

                // Solid core out to core_radius, then smooth falloff to the edge.
                constexpr float core_radius = 0.65f;
                float a;
                if (d <= core_radius)
                {
                    a = 1.0f;
                }
                else if (d < 1.0f)
                {
                    const float t = (d - core_radius) / (1.0f - core_radius);
                    a = 1.0f - t * t;
                }
                else
                {
                    a = 0.0f;
                }

                const size_t i = (static_cast<size_t>(y) * size + x) * 4;
                pixels[i + 0] = 0;
                pixels[i + 1] = 0;
                pixels[i + 2] = 0;
                pixels[i + 3] = static_cast<uint8_t>(a * 255.0f);
            }
        }
        return pixels;
    }
}
