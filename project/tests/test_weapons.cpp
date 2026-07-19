#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "weapons/FireEdge.h"
#include "weapons/FireRate.h"
#include "weapons/Pellets.h"
#include "weapons/WeaponDef.h"
#include "weapons/WeaponRegistry.h"
#include "weapons/WeaponRuntime.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <set>

using Catch::Approx;
using namespace weapons;

// ---- Registry & settings ----

TEST_CASE("WeaponRegistry: every WeaponId resolves to a registered WeaponDef") {
    const WeaponRegistry& reg = registry();

    const WeaponDef& gun     = reg.def(WeaponId::BasicGun);
    const WeaponDef& pistol  = reg.def(WeaponId::BasicPistol);
    const WeaponDef& shotgun = reg.def(WeaponId::BasicShotgun);

    REQUIRE_FALSE(gun.name.empty());
    REQUIRE_FALSE(pistol.name.empty());
    REQUIRE_FALSE(shotgun.name.empty());
}

TEST_CASE("WeaponRegistry: names are non-empty and unique") {
    const WeaponRegistry& reg = registry();
    std::set<std::string> names;
    for (auto id : { WeaponId::BasicGun, WeaponId::BasicPistol, WeaponId::BasicShotgun })
    {
        const WeaponDef& d = reg.def(id);
        REQUIRE_FALSE(d.name.empty());
        REQUIRE(names.insert(d.name).second); // insert returns false on duplicate
    }
}

TEST_CASE("WeaponRegistry: model paths are set") {
    const WeaponRegistry& reg = registry();
    REQUIRE_FALSE(reg.def(WeaponId::BasicGun).modelPath.empty());
    REQUIRE_FALSE(reg.def(WeaponId::BasicPistol).modelPath.empty());
    REQUIRE_FALSE(reg.def(WeaponId::BasicShotgun).modelPath.empty());
}

TEST_CASE("WeaponRegistry: pistol is single-pellet, shotgun is multi-pellet") {
    const WeaponRegistry& reg = registry();
    REQUIRE(reg.def(WeaponId::BasicPistol).pelletsPerShot == 1);
    REQUIRE(reg.def(WeaponId::BasicShotgun).pelletsPerShot > 1);
}

TEST_CASE("WeaponRegistry: fire modes match expected weapon archetypes") {
    const WeaponRegistry& reg = registry();
    REQUIRE(reg.def(WeaponId::BasicPistol).fireMode  == FireMode::Semi);
    REQUIRE(reg.def(WeaponId::BasicShotgun).fireMode == FireMode::Semi);
    REQUIRE(reg.def(WeaponId::BasicGun).fireMode     == FireMode::Auto);
}

TEST_CASE("WeaponRegistry: settings are all positive/sane") {
    const WeaponRegistry& reg = registry();
    for (auto id : { WeaponId::BasicGun, WeaponId::BasicPistol, WeaponId::BasicShotgun })
    {
        const WeaponDef& d = reg.def(id);
        REQUIRE(d.magazineCapacity > 0);
        REQUIRE(d.reserveAmmoMax   > 0);
        REQUIRE(d.reloadSeconds    > 0.0f);
        REQUIRE(d.damage           > 0);
        REQUIRE(d.fireRate         > 0.0f);
        REQUIRE(d.pelletsPerShot   > 0);
        REQUIRE(d.spreadDegrees    >= 0.0f);
    }
}

// ---- HUD capacity derivation ----
// The HUD never receives magazineCapacity/reserveAmmoMax on the wire - it looks them up
// via the replicated equippedWeapon id. Verify the same id always resolves to the same
// capacity values (not specific numbers, which change often as weapons are tuned).
TEST_CASE("HUD capacity derivation: equippedWeapon resolves to a stable magazineCapacity/reserveAmmoMax") {
    const auto equipped = WeaponId::BasicPistol;
    const WeaponDef& a  = registry().def(equipped);
    const WeaponDef& b  = registry().def(equipped);
    REQUIRE(a.magazineCapacity == b.magazineCapacity);
    REQUIRE(a.reserveAmmoMax   == b.reserveAmmoMax);
}

TEST_CASE("WeaponRegistry: out-of-range WeaponId returns a defined fallback, not UB") {
    const WeaponRegistry& reg     = registry();
    const auto            invalid = static_cast<WeaponId>(255);
    const WeaponDef&      d       = reg.def(invalid);

    REQUIRE(d.name.empty());
    REQUIRE(d.damage == 0);
}

// ---- Fire rate ----

TEST_CASE("ticks_per_shot: converts rounds/second at a given tick rate") {
    REQUIRE(ticks_per_shot(10.0f, 60.0f) == 6u);
    REQUIRE(ticks_per_shot(6.0f, 60.0f)  == 10u);
    REQUIRE(ticks_per_shot(60.0f, 60.0f) == 1u);
}

TEST_CASE("ticks_per_shot: degenerate rates are clamped, not divide-by-zero") {
    REQUIRE(ticks_per_shot(0.0f, 60.0f)   == UINT32_MAX);
    REQUIRE(ticks_per_shot(-5.0f, 60.0f)  == UINT32_MAX);
    REQUIRE(ticks_per_shot(1000.0f, 60.0f) == 1u); // faster than tick rate clamps to 1
}

TEST_CASE("WeaponRuntime::can_fire: false before the interval elapses, true on/after it") {
    WeaponDef def;
    def.magazineCapacity = 10;
    def.reserveAmmoMax   = 10;
    def.fireRate         = 10.0f; // -> 6 ticks @ 60 Hz

    WeaponRuntime rt;
    rt.init(def);
    rt.lastFireTick = 100;
    rt.hasFired     = true;

    REQUIRE_FALSE(rt.can_fire(103, def, 60.0f)); // 3 ticks elapsed - too soon
    REQUIRE_FALSE(rt.can_fire(105, def, 60.0f)); // 5 ticks elapsed - still too soon
    REQUIRE(rt.can_fire(106, def, 60.0f));       // exactly 6 ticks - boundary, allowed
    REQUIRE(rt.can_fire(107, def, 60.0f));       // past the boundary
}

// ---- Ammo & reload ----

TEST_CASE("WeaponRuntime::on_fire decrements ammo; can_fire false at zero without reserve") {
    WeaponDef def;
    def.magazineCapacity = 2;
    def.reserveAmmoMax   = 0;
    def.fireRate         = 1000.0f; // effectively unlimited rate for this test

    WeaponRuntime rt;
    rt.init(def);
    REQUIRE(rt.ammoInMag == 2);

    rt.on_fire(0, def);
    REQUIRE(rt.ammoInMag == 1);
    REQUIRE(rt.can_fire(1, def));

    rt.on_fire(1, def);
    REQUIRE(rt.ammoInMag == 0);
    REQUIRE_FALSE(rt.can_fire(2, def));
    // No reserve - firing the last round must not auto-schedule a reload.
    REQUIRE(rt.reloadFinishTick == 0);
}

TEST_CASE("WeaponRuntime: auto-reload on empty schedules only when reserve > 0") {
    WeaponDef def;
    def.magazineCapacity = 1;
    def.reserveAmmoMax   = 5;
    def.reloadSeconds    = 2.0f;
    def.fireRate         = 1000.0f;

    WeaponRuntime rt;
    rt.init(def);
    REQUIRE(rt.reserveAmmo == 5);

    rt.on_fire(10, def, 60.0f); // empties the single-round magazine, reserve=5 > 0
    REQUIRE(rt.ammoInMag == 0);
    REQUIRE(rt.reloadFinishTick == 10 + static_cast<uint32_t>(2.0f * 60.0f));
}

TEST_CASE("WeaponRuntime::request_reload: no-op while already reloading, full, or empty reserve") {
    WeaponDef def;
    def.magazineCapacity = 5;
    def.reserveAmmoMax   = 10;
    def.reloadSeconds    = 1.0f;

    WeaponRuntime rt;
    rt.init(def); // full magazine, full reserve

    REQUIRE_FALSE(rt.request_reload(0, def, 60.0f)); // magazine already full

    rt.ammoInMag = 3;
    REQUIRE(rt.request_reload(0, def, 60.0f)); // accepted
    REQUIRE(rt.reloadFinishTick == 60u);

    // Already reloading - a second request must not restart/extend it.
    REQUIRE_FALSE(rt.request_reload(5, def, 60.0f));
    REQUIRE(rt.reloadFinishTick == 60u);

    // Complete the reload, then drain the reserve and confirm empty-reserve rejects.
    rt.advance(60, def);
    rt.reserveAmmo = 0;
    rt.ammoInMag   = 0;
    REQUIRE_FALSE(rt.request_reload(60, def, 60.0f));
}

TEST_CASE("WeaponRuntime::can_fire is false for the whole reload window and true once it completes") {
    WeaponDef def;
    def.magazineCapacity = 5;
    def.reserveAmmoMax   = 10;
    def.reloadSeconds    = 1.0f;
    def.fireRate         = 1000.0f;

    WeaponRuntime rt;
    rt.init(def);
    rt.ammoInMag = 0; // empty, but keep enough gap since fire rate is high

    REQUIRE(rt.request_reload(0, def, 60.0f));
    REQUIRE(rt.reloadFinishTick == 60u);

    for (uint32_t t = 0; t < 60; ++t)
    {
        REQUIRE_FALSE(rt.can_fire(t, def, 60.0f));
    }

    rt.advance(60, def);
    REQUIRE(rt.reloadFinishTick == 0u);
    REQUIRE(rt.can_fire(60, def, 60.0f));
}

TEST_CASE("WeaponRuntime::advance: full reserve refills to capacity exactly") {
    WeaponDef def;
    def.magazineCapacity = 10;
    def.reserveAmmoMax   = 50;

    WeaponRuntime rt;
    rt.ammoInMag         = 2;
    rt.reserveAmmo       = 50;
    rt.reloadFinishTick  = 100;

    rt.advance(99, def); // not yet
    REQUIRE(rt.ammoInMag == 2);

    rt.advance(100, def); // finish tick reached
    REQUIRE(rt.ammoInMag   == 10);
    REQUIRE(rt.reserveAmmo == 42);
    REQUIRE(rt.reloadFinishTick == 0u);
}

TEST_CASE("WeaponRuntime::advance: reserve smaller than the missing rounds gives a partial magazine") {
    WeaponDef def;
    def.magazineCapacity = 10;
    def.reserveAmmoMax   = 50;

    WeaponRuntime rt;
    rt.ammoInMag        = 2;
    rt.reserveAmmo       = 3; // fewer than the 8 missing rounds
    rt.reloadFinishTick  = 50;

    rt.advance(50, def);
    REQUIRE(rt.ammoInMag   == 5);  // 2 + 3
    REQUIRE(rt.reserveAmmo == 0);
    REQUIRE(rt.reloadFinishTick == 0u);
}

TEST_CASE("WeaponRuntime::add_reserve clamps to reserveAmmoMax") {
    WeaponDef def;
    def.reserveAmmoMax = 50;

    WeaponRuntime rt;
    rt.reserveAmmo = 40;

    rt.add_reserve(20, def); // 40 + 20 = 60, clamp to 50
    REQUIRE(rt.reserveAmmo == 50);
}

// ---- Semi vs full auto ----

TEST_CASE("should_fire: Semi fires once per press-edge while held") {
    WeaponDef def;
    def.fireMode = FireMode::Semi;
    def.fireRate = 60.0f; // 1 tick between shots @ 60 Hz - fast enough that the rate
                          // gate doesn't interfere with these well-separated presses.

    FireEdgeState state;
    uint32_t tick = 0;

    REQUIRE(weapons::should_fire(state, true, tick++, def, 60.0f));   // press-edge: fires
    REQUIRE_FALSE(weapons::should_fire(state, true, tick++, def, 60.0f)); // still held: no repeat
    REQUIRE_FALSE(weapons::should_fire(state, true, tick++, def, 60.0f)); // still held: no repeat
    REQUIRE_FALSE(weapons::should_fire(state, false, tick++, def, 60.0f)); // released
    REQUIRE(weapons::should_fire(state, true, tick++, def, 60.0f));   // pressed again: fires
}

TEST_CASE("should_fire: Semi is also gated by the weapon's fire rate (matches server's can_fire)") {
    // Regression: the server's can_fire() rate-gates every shot regardless of fire mode
    // (a gun's mechanical cycle time limits Semi too), so a fast re-press on a Semi
    // weapon must not predict a shot the server will drop.
    WeaponDef def;
    def.fireMode = FireMode::Semi;
    def.fireRate = 4.0f; // 15 ticks between shots @ 60 Hz

    FireEdgeState state;

    REQUIRE(weapons::should_fire(state, true, 0, def, 60.0f));         // first press: fires
    REQUIRE_FALSE(weapons::should_fire(state, false, 1, def, 60.0f));  // release
    REQUIRE_FALSE(weapons::should_fire(state, true, 2, def, 60.0f));   // re-press too soon (2 ticks) - blocked
    REQUIRE_FALSE(weapons::should_fire(state, false, 3, def, 60.0f));  // release
    REQUIRE(weapons::should_fire(state, true, 15, def, 60.0f));        // re-press at the boundary - fires
}

TEST_CASE("should_fire: Auto fires repeatedly at the fire rate while held") {
    WeaponDef def;
    def.fireMode = FireMode::Auto;
    def.fireRate = 10.0f; // 6 ticks @ 60 Hz between shots

    FireEdgeState state;
    int shots = 0;
    for (uint32_t t = 0; t < 60; ++t)
    {
        if (weapons::should_fire(state, true, t, def, 60.0f))
        {
            ++shots;
        }
    }
    // 60 ticks / 6-tick interval, firing on tick 0 then every 6 ticks after -> 10 shots.
    REQUIRE(shots == 10);
}

// ---- Spread / pellets ----

TEST_CASE("pellet_directions: returns exactly pelletsPerShot directions") {
    WeaponDef def;
    def.pelletsPerShot = 8;
    def.spreadDegrees  = 5.0f;

    std::mt19937 rng(1234);
    const auto pellets = pellet_directions(glm::vec3(0.0f, 0.0f, -1.0f), def, rng);
    REQUIRE(pellets.size() == 8);
}

TEST_CASE("pellet_directions: single-pellet zero-spread returns the aim direction unchanged") {
    WeaponDef def;
    def.pelletsPerShot = 1;
    def.spreadDegrees  = 0.0f;

    const glm::vec3 aim(0.0f, 0.0f, -1.0f);
    std::mt19937 rng(7);
    const auto pellets = pellet_directions(aim, def, rng);

    REQUIRE(pellets.size() == 1);
    REQUIRE(pellets[0].x == aim.x);
    REQUIRE(pellets[0].y == aim.y);
    REQUIRE(pellets[0].z == aim.z);
}

TEST_CASE("pellet_directions: every pellet lies within spreadDegrees of the aim and is normalized") {
    WeaponDef def;
    def.pelletsPerShot = 20;
    def.spreadDegrees  = 8.0f;

    const glm::vec3 aim = glm::normalize(glm::vec3(0.3f, 0.1f, -1.0f));
    std::mt19937 rng(99);
    const auto pellets = pellet_directions(aim, def, rng);

    const float max_angle_rad = glm::radians(def.spreadDegrees) + 1e-3f;
    for (const auto& p : pellets)
    {
        REQUIRE(std::abs(glm::length(p) - 1.0f) < 1e-4f);
        const float cos_angle = std::clamp(glm::dot(aim, p), -1.0f, 1.0f);
        const float angle     = std::acos(cos_angle);
        REQUIRE(angle <= max_angle_rad);
    }
}

TEST_CASE("pellet_directions: deterministic for a fixed RNG seed") {
    WeaponDef def;
    def.pelletsPerShot = 6;
    def.spreadDegrees  = 6.0f;

    const glm::vec3 aim(0.0f, 0.0f, -1.0f);

    std::mt19937 rngA(42);
    std::mt19937 rngB(42);
    const auto pelletsA = pellet_directions(aim, def, rngA);
    const auto pelletsB = pellet_directions(aim, def, rngB);

    REQUIRE(pelletsA.size() == pelletsB.size());
    for (size_t i = 0; i < pelletsA.size(); ++i)
    {
        REQUIRE(pelletsA[i].x == Approx(pelletsB[i].x));
        REQUIRE(pelletsA[i].y == Approx(pelletsB[i].y));
        REQUIRE(pelletsA[i].z == Approx(pelletsB[i].z));
    }
}
