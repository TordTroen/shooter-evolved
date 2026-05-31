#include "ui/Hud.h"

#include <catch2/catch_test_macros.hpp>

// draw() requires an active ImGui frame and is not tested here.
// These tests cover only the timer-state logic.

TEST_CASE("Hud hitmarker is inactive by default")
{
    Hud hud;
    REQUIRE_FALSE(hud.hitmarkerActive());
}

TEST_CASE("Hud hitmarker becomes active after triggerHitmarker()")
{
    Hud hud;
    hud.triggerHitmarker();
    REQUIRE(hud.hitmarkerActive());
}
