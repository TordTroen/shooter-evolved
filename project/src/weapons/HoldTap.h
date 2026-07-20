#pragma once

namespace weapons
{
    constexpr int kHoldTicks = 30; // 0.5 s @ 60 Hz - pickup/drop hold threshold

    struct HoldTapState
    {
        int  heldTicks = 0;
        bool latched   = false; // hold action already fired for this press
    };

    struct HoldTapEdges
    {
        bool tap       = false;
        bool holdStart = false;
    };

    // Turns a per-tick pressed/not-pressed signal into tap-vs-hold action edges, independent
    // of SDL/gamepad. Released before crossing hold_ticks -> `tap` fires once, on release.
    // Held past hold_ticks -> `holdStart` fires once, on the crossing tick, then latches
    // (no repeat) until release.
    HoldTapEdges update_hold_tap(HoldTapState& state, bool pressed, int hold_ticks = kHoldTicks);
}
