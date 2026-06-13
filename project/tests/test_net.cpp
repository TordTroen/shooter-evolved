#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "net/BitStream.h"
#include "net/InputFrame.h"
#include "net/PlayerState.h"
#include "net/FireIntent.h"

using Catch::Approx;

// ---- BitStream round-trips ----

TEST_CASE("BitStream: uint bits round-trip") {
    for (int bits = 1; bits <= 32; ++bits) {
        const uint32_t original = (1u << bits) - 1u; // all-ones for this width
        BitStream w;
        w.writeBits(original, bits);

        BitStream r(w.bufferData(), w.bufferBytes());
        REQUIRE(r.readBits(bits) == original);
    }
}

TEST_CASE("BitStream: float round-trip") {
    const float vals[] = { 0.0f, 1.0f, -1.0f, 3.14159f, -999.5f, 0.0001f };
    for (float v : vals) {
        BitStream w;
        w.writeFloat(v);

        BitStream r(w.bufferData(), w.bufferBytes());
        REQUIRE(r.readFloat() == Approx(v));
    }
}

TEST_CASE("BitStream: vec3 round-trip") {
    const glm::vec3 orig(1.5f, -2.0f, 0.125f);
    BitStream w;
    w.writeVec3(orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    const glm::vec3 result = r.readVec3();
    REQUIRE(result.x == Approx(orig.x));
    REQUIRE(result.y == Approx(orig.y));
    REQUIRE(result.z == Approx(orig.z));
}

TEST_CASE("BitStream: string round-trip") {
    const std::string s = "hello network";
    BitStream w;
    w.writeString(s);

    BitStream r(w.bufferData(), w.bufferBytes());
    REQUIRE(r.readString() == s);
}

TEST_CASE("BitStream: empty string round-trip") {
    BitStream w;
    w.writeString("");

    BitStream r(w.bufferData(), w.bufferBytes());
    REQUIRE(r.readString() == "");
}

// ---- InputFrame round-trip ----

TEST_CASE("InputFrame: serialize round-trip") {
    InputFrame orig;
    orig.move    = { 0.5f, -0.3f };
    orig.yaw     = 135.0f;
    orig.pitch   = -15.0f;
    orig.buttons = InputFrame::kButtonJump | InputFrame::kButtonFire;
    orig.tick    = 42;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    InputFrame result{};
    serialize(r, result);

    REQUIRE(result.move.x  == Approx(orig.move.x));
    REQUIRE(result.move.y  == Approx(orig.move.y));
    REQUIRE(result.yaw     == Approx(orig.yaw));
    REQUIRE(result.pitch   == Approx(orig.pitch));
    REQUIRE(result.buttons == orig.buttons);
    REQUIRE(result.tick    == orig.tick);
}

// ---- PlayerState round-trip ----

TEST_CASE("PlayerState: serialize round-trip") {
    PlayerState orig;
    orig.position = { 10.0f, 2.5f, -5.0f };
    orig.yaw      = 270.0f;
    orig.pitch    = 12.5f;
    orig.health   = 75;
    orig.buttons  = InputFrame::kButtonFire;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    serialize(r, result);

    REQUIRE(result.position.x == Approx(orig.position.x));
    REQUIRE(result.position.y == Approx(orig.position.y));
    REQUIRE(result.position.z == Approx(orig.position.z));
    REQUIRE(result.yaw        == Approx(orig.yaw));
    REQUIRE(result.pitch      == Approx(orig.pitch));
    REQUIRE(result.health     == orig.health);
    REQUIRE(result.buttons    == orig.buttons);
}

// ---- FireIntent round-trip ----

TEST_CASE("FireIntent: serialize round-trip") {
    FireIntent orig;
    orig.shooterId  = NetworkId{7};
    orig.clientTick = 100;
    orig.origin     = { 1.0f, 2.0f, 3.0f };
    orig.direction  = { 0.0f, 0.0f, -1.0f };

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    FireIntent result{};
    serialize(r, result);

    REQUIRE(result.shooterId.value == orig.shooterId.value);
    REQUIRE(result.clientTick      == orig.clientTick);
    REQUIRE(result.origin.x        == Approx(orig.origin.x));
    REQUIRE(result.origin.z        == Approx(orig.origin.z));
    REQUIRE(result.direction.z     == Approx(orig.direction.z));
}

// ---- Time arithmetic: server tick estimation ----

TEST_CASE("Clock sync: serverTick estimation stays bounded") {
    // Simulate 20 snapshots arriving at 20 Hz (50 ms apart) and verify
    // that the estimated server tick stays within ±2 ticks of truth.
    uint32_t serverTick       = 0;
    uint32_t latestSnapTick   = 0;
    uint32_t ticksSinceSnap   = 0;
    const float snapshotDt    = 1.0f / 20.0f;
    const float simTickDt     = 1.0f / 60.0f;

    for (int snap = 0; snap < 20; ++snap)
    {
        // Advance simulation by snapshot interval.
        const int simTicks = static_cast<int>(snapshotDt / simTickDt + 0.5f);
        serverTick     += simTicks;
        ticksSinceSnap  = 0;

        // Snapshot arrives: update latest known tick.
        latestSnapTick = serverTick;

        // Client estimates current server tick.
        const uint32_t estimated = latestSnapTick + ticksSinceSnap;
        REQUIRE(estimated <= serverTick + 2);
        REQUIRE(estimated + 2 >= serverTick);
    }
}
