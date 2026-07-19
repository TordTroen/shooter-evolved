#pragma once

#include "FireRate.h"
#include "WeaponDef.h"

#include <algorithm>
#include <cstdint>

namespace weapons
{
    // Per-player firing state machine: ammo, fire-rate gating, and reload. Pure - no
    // Server/Scene dependency, so it can be unit-tested headlessly.
    struct WeaponRuntime
    {
        int      ammoInMag       = 0;
        int      reserveAmmo     = 0;
        uint32_t lastFireTick    = 0;
        bool     hasFired        = false; // guards the fire-rate gate before the first shot
        uint32_t reloadFinishTick = 0; // 0 = not reloading

        // Starts with a full magazine and full reserve.
        void init(const WeaponDef& def)
        {
            ammoInMag   = def.magazineCapacity;
            reserveAmmo = def.reserveAmmoMax;
            lastFireTick     = 0;
            hasFired         = false;
            reloadFinishTick = 0;
        }

        [[nodiscard]] bool can_fire(uint32_t now, const WeaponDef& def, float tick_rate = 60.0f) const
        {
            if (reloadFinishTick != 0)      { return false; } // mid-reload
            if (ammoInMag <= 0)              { return false; }
            // hasFired guards the very first shot - lastFireTick defaults to 0, which
            // would otherwise falsely look "too soon" for any now close to tick 0.
            if (hasFired && now - lastFireTick < ticks_per_shot(def.fireRate, tick_rate)) { return false; }
            return true;
        }

        // Caller must have checked can_fire(). Decrements ammo and, if the magazine just
        // emptied and reserve remains, auto-schedules a reload.
        void on_fire(uint32_t now, const WeaponDef& def, float tick_rate = 60.0f)
        {
            lastFireTick = now;
            hasFired     = true;
            --ammoInMag;
            if (ammoInMag <= 0 && reserveAmmo > 0)
            {
                request_reload(now, def, tick_rate);
            }
        }

        // No-op (returns false) when already reloading, when the magazine is already
        // full, or when the reserve is empty.
        bool request_reload(uint32_t now, const WeaponDef& def, float tick_rate = 60.0f)
        {
            if (reloadFinishTick != 0)             { return false; }
            if (ammoInMag >= def.magazineCapacity) { return false; }
            if (reserveAmmo <= 0)                  { return false; }

            const uint32_t duration = static_cast<uint32_t>(def.reloadSeconds * tick_rate + 0.5f);
            reloadFinishTick = now + duration;
            return true;
        }

        // Completes the reload once its finish tick has passed. Partial refill when the
        // reserve is smaller than the missing rounds.
        void advance(uint32_t now, const WeaponDef& def)
        {
            if (reloadFinishTick == 0 || now < reloadFinishTick) { return; }

            const int moved = std::min(def.magazineCapacity - ammoInMag, reserveAmmo);
            ammoInMag        += moved;
            reserveAmmo      -= moved;
            reloadFinishTick  = 0;
        }

        // Adds reserve ammo (e.g. picking up the same weapon), clamped to reserveAmmoMax.
        void add_reserve(int amount, const WeaponDef& def)
        {
            reserveAmmo = std::min(reserveAmmo + amount, def.reserveAmmoMax);
        }
    };
}
