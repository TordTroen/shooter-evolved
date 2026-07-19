#pragma once

#include <cstdint>

namespace weapons
{
    // Converts a fire rate (rounds/second) into a minimum tick interval between shots.
    // Degenerate rates are clamped rather than dividing by zero: fire_rate <= 0 means
    // "never fires again on its own" (returns the largest representable interval);
    // fire_rate faster than the tick rate is clamped to the minimum interval of 1 tick.
    [[nodiscard]] inline uint32_t ticks_per_shot(float fire_rate, float tick_rate)
    {
        if (fire_rate <= 0.0f)
        {
            return UINT32_MAX;
        }

        const float ticks = tick_rate / fire_rate;
        if (ticks <= 1.0f)
        {
            return 1u;
        }

        return static_cast<uint32_t>(ticks + 0.5f); // round to nearest
    }
}
