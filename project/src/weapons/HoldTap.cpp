#include "HoldTap.h"

namespace weapons
{
    HoldTapEdges update_hold_tap(HoldTapState& state, bool pressed, int hold_ticks)
    {
        HoldTapEdges edges;

        if (pressed)
        {
            ++state.heldTicks;
            if (!state.latched && state.heldTicks >= hold_ticks)
            {
                state.latched   = true;
                edges.holdStart = true;
            }
        }
        else
        {
            if (state.heldTicks > 0 && !state.latched)
            {
                edges.tap = true;
            }
            state.heldTicks = 0;
            state.latched   = false;
        }

        return edges;
    }
}
