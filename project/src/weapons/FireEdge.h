#pragma once

#include "FireRate.h"
#include "WeaponDef.h"

#include <cstdint>

namespace weapons
{
    // Tracks the client-side trigger edge across frames so should_fire() can be pure.
    struct FireEdgeState
    {
        bool     wasHeld      = false;
        uint32_t lastFireTick = 0;
        bool     hasFired     = false; // guards the fire-rate gate before the first shot
    };

    // Semi: one shot per press-edge (held transitioning false -> true).
    // Auto: fires every tick the button is held.
    // Both are additionally gated by the weapon's fire rate - the server's can_fire()
    // rate-gates every shot regardless of fire mode (a gun's mechanical cycle time
    // limits Semi too), so the cosmetic prediction must match or a fast double-click
    // shows a muzzle flash/FireIntent the server silently drops.
    [[nodiscard]] inline bool should_fire(FireEdgeState& state, bool held, uint32_t now,
                                          const WeaponDef& def, float tick_rate = 60.0f)
    {
        const bool press_edge = held && !state.wasHeld;
        state.wasHeld = held;

        bool wants_fire = (def.fireMode == FireMode::Semi) ? press_edge : held;
        if (wants_fire && state.hasFired &&
            (now - state.lastFireTick) < ticks_per_shot(def.fireRate, tick_rate))
        {
            wants_fire = false;
        }

        if (wants_fire)
        {
            state.lastFireTick = now;
            state.hasFired     = true;
        }
        return wants_fire;
    }
}
