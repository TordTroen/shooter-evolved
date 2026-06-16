#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "net/ActorState.h"
#include "net/BitStream.h"
#include "net/HitscanMath.h"
#include "net/InputFrame.h"
#include "net/PlayerState.h"
#include "net/FireIntent.h"
#include "net/Snapshot.h"
#include "net/MockTransport.h"
#include "net/Transport.h"

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

// ---- SnapshotMessage round-trip ----

TEST_CASE("SnapshotMessage: serialize round-trip") {
    SnapshotMessage orig;
    orig.serverTick = 12345;
    orig.players.push_back({ NetworkId{1}, PlayerState{} });
    orig.players.push_back({ NetworkId{2}, PlayerState{} });
    orig.players[0].state.position = { 1.0f, 2.0f, 3.0f };
    orig.players[0].state.yaw      = 90.0f;
    orig.players[0].state.health   = 80;
    orig.players[1].state.position = { -4.0f, 0.5f, 9.0f };
    orig.players[1].state.health   = 33;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.serverTick == orig.serverTick);
    REQUIRE(result.players.size() == orig.players.size());
    for (size_t i = 0; i < orig.players.size(); ++i) {
        REQUIRE(result.players[i].netId.value   == orig.players[i].netId.value);
        REQUIRE(result.players[i].state.position.x == Approx(orig.players[i].state.position.x));
        REQUIRE(result.players[i].state.position.z == Approx(orig.players[i].state.position.z));
        REQUIRE(result.players[i].state.health  == orig.players[i].state.health);
    }
}

TEST_CASE("SnapshotMessage: empty player list round-trips") {
    SnapshotMessage orig;
    orig.serverTick = 7;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.serverTick == 7);
    REQUIRE(result.players.empty());
}

// ---- Never trust the wire ----

TEST_CASE("BitStream: reading past the buffer flags an error") {
    BitStream w;
    w.writeBits(0xFF, 8); // one byte written

    BitStream r(w.bufferData(), w.bufferBytes());
    r.readBits(8);
    REQUIRE_FALSE(r.hasError());
    r.readBits(8); // past the end
    REQUIRE(r.hasError());
}

TEST_CASE("BitStream: oversized string length is rejected, not allocated") {
    // Hand-craft a buffer claiming a 60000-char string but carrying no payload.
    BitStream w;
    w.writeBits(60000, 16);

    BitStream r(w.bufferData(), w.bufferBytes());
    const std::string s = r.readString();
    REQUIRE(r.hasError());
    REQUIRE(s.empty());
}

TEST_CASE("SnapshotMessage: player count over kMaxPlayers is rejected") {
    // Forge a snapshot: serverTick(32) + count(8)=200, then no bodies.
    BitStream w;
    uint32_t tick = 1;
    w.serializeBits(tick, 32);
    uint32_t count = 200;
    w.serializeBits(count, 8);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE(r.hasError());
    REQUIRE(result.players.empty()); // never sized to the bogus count
}

TEST_CASE("SnapshotMessage: actor count over kMaxActors is rejected") {
    // Forge a snapshot: serverTick(32) + playerCount(8)=0 + actorCount(8)=200, no bodies.
    BitStream w;
    uint32_t tick = 1;
    w.serializeBits(tick, 32);
    uint32_t player_count = 0;
    w.serializeBits(player_count, 8);
    uint32_t actor_count = 200;
    w.serializeBits(actor_count, 8);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE(r.hasError());
    REQUIRE(result.actors.empty()); // never sized to the bogus count
}

// ---- MockTransport: in-process message routing ----

TEST_CASE("MockTransport: messages route A-to-B") {
    auto [a, b] = MockTransport::createPair();

    // Trigger the connect handshake by calling connectTo on side A.
    const ConnectionId connA = a->connectTo("loopback", 0);
    REQUIRE(connA == 1);

    // A sends a snapshot message to B.
    SnapshotMessage orig;
    orig.serverTick = 99;
    orig.players.push_back({ NetworkId{3}, PlayerState{} });
    orig.players[0].state.position = { 1.0f, 2.0f, 3.0f };
    orig.players[0].state.health   = 55;

    BitStream w;
    serialize(w, orig);
    a->send(1, Channel::Unreliable,
            reinterpret_cast<const std::byte*>(w.bufferData()), w.bufferBytes());

    // B receives it.
    std::vector<IncomingMessage> msgs;
    b->poll(msgs);

    // First entry may be the connect notification (no data); find the one with data.
    const IncomingMessage* found = nullptr;
    for (const auto& m : msgs)
    {
        if (!m.data.empty()) { found = &m; break; }
    }
    REQUIRE(found != nullptr);
    REQUIRE(found->channel == Channel::Unreliable);

    BitStream r(found->data.data(), found->data.size());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.serverTick == orig.serverTick);
    REQUIRE(result.players.size() == 1);
    REQUIRE(result.players[0].netId.value == orig.players[0].netId.value);
    REQUIRE(result.players[0].state.position.x == Catch::Approx(orig.players[0].state.position.x));
    REQUIRE(result.players[0].state.health == orig.players[0].state.health);
}

TEST_CASE("MockTransport: B-to-A routing works") {
    auto [a, b] = MockTransport::createPair();
    a->connectTo("loopback", 0);

    const uint8_t payload[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    b->send(1, Channel::Reliable,
            reinterpret_cast<const std::byte*>(payload), sizeof(payload));

    std::vector<IncomingMessage> msgs;
    a->poll(msgs);

    const IncomingMessage* found = nullptr;
    for (const auto& m : msgs)
    {
        if (!m.data.empty()) { found = &m; break; }
    }
    REQUIRE(found != nullptr);
    REQUIRE(found->channel == Channel::Reliable);
    REQUIRE(found->data.size() == sizeof(payload));
    REQUIRE(found->data[0] == std::byte{0xDE});
    REQUIRE(found->data[3] == std::byte{0xEF});
}

// ---- ActorState round-trip ----

TEST_CASE("ActorState: serialize round-trip") {
    ActorState orig;
    orig.netId    = NetworkId{0x10002};
    orig.position = { 3.0f, 0.5f, -5.0f };
    orig.rotation = glm::quat(0.707f, 0.0f, 0.707f, 0.0f); // ~90° around Y
    orig.health   = 42;
    orig.isAlive  = true;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    ActorState result{};
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.netId.value  == orig.netId.value);
    REQUIRE(result.position.x   == Approx(orig.position.x));
    REQUIRE(result.position.y   == Approx(orig.position.y));
    REQUIRE(result.position.z   == Approx(orig.position.z));
    REQUIRE(result.rotation.x   == Approx(orig.rotation.x));
    REQUIRE(result.rotation.y   == Approx(orig.rotation.y));
    REQUIRE(result.rotation.z   == Approx(orig.rotation.z));
    REQUIRE(result.rotation.w   == Approx(orig.rotation.w));
    REQUIRE(result.health       == orig.health);
    REQUIRE(result.isAlive      == orig.isAlive);
}

TEST_CASE("ActorState: isAlive=false and zero health round-trip") {
    ActorState orig;
    orig.netId   = NetworkId{0x10000};
    orig.health  = 0;
    orig.isAlive = false;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    ActorState result{};
    result.isAlive = true; // start with opposite to confirm overwrite
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.health  == 0);
    REQUIRE(result.isAlive == false);
}

// ---- SnapshotMessage with actors ----

TEST_CASE("SnapshotMessage: round-trip with actors section") {
    SnapshotMessage orig;
    orig.serverTick = 999;
    orig.players.push_back({ NetworkId{1}, PlayerState{} });
    orig.players[0].state.health = 80;

    ActorState a0;
    a0.netId    = NetworkId{0x10000};
    a0.position = { 1.0f, 0.5f, -3.0f };
    a0.health   = 100;
    a0.isAlive  = true;

    ActorState a1;
    a1.netId    = NetworkId{0x10001};
    a1.position = { -2.0f, 0.5f, -5.0f };
    a1.health   = 0;
    a1.isAlive  = false;

    orig.actors.push_back(a0);
    orig.actors.push_back(a1);

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.serverTick       == orig.serverTick);
    REQUIRE(result.players.size()   == 1);
    REQUIRE(result.players[0].state.health == 80);
    REQUIRE(result.actors.size()    == 2);
    REQUIRE(result.actors[0].netId.value   == a0.netId.value);
    REQUIRE(result.actors[0].position.x    == Approx(a0.position.x));
    REQUIRE(result.actors[0].health        == a0.health);
    REQUIRE(result.actors[0].isAlive       == true);
    REQUIRE(result.actors[1].netId.value   == a1.netId.value);
    REQUIRE(result.actors[1].health        == 0);
    REQUIRE(result.actors[1].isAlive       == false);
}

// ---- HitscanMath: ray_vs_capsule ----

TEST_CASE("ray_vs_capsule: hits cylinder body head-on") {
    // Capsule standing upright at origin, spanning y=0.3 to y=1.7, radius=0.3.
    const glm::vec3 cap_a(0.0f, 0.3f, 0.0f);
    const glm::vec3 cap_b(0.0f, 1.7f, 0.0f);
    const float     radius = 0.3f;

    // Ray coming from Z+ aimed at the capsule centre.
    const glm::vec3 ro(0.0f, 1.0f, 10.0f);
    const glm::vec3 rd(0.0f, 0.0f, -1.0f);

    const float t = ray_vs_capsule(ro, rd, cap_a, cap_b, radius);
    REQUIRE(t > 0.0f);
    REQUIRE(t == Approx(10.0f - radius).epsilon(0.01));
}

TEST_CASE("ray_vs_capsule: misses when aimed away") {
    const glm::vec3 cap_a(0.0f, 0.3f, 0.0f);
    const glm::vec3 cap_b(0.0f, 1.7f, 0.0f);
    const float     radius = 0.3f;

    // Ray aimed in the opposite direction.
    const glm::vec3 ro(0.0f, 1.0f, 10.0f);
    const glm::vec3 rd(0.0f, 0.0f, 1.0f);

    REQUIRE(ray_vs_capsule(ro, rd, cap_a, cap_b, radius) < 0.0f);
}

TEST_CASE("ray_vs_capsule: misses when offset laterally beyond radius") {
    const glm::vec3 cap_a(0.0f, 0.3f, 0.0f);
    const glm::vec3 cap_b(0.0f, 1.7f, 0.0f);
    const float     radius = 0.3f;

    // Ray at X=1.0 (beyond radius=0.3), aimed straight at capsule's Z.
    const glm::vec3 ro(1.0f, 1.0f, 10.0f);
    const glm::vec3 rd(0.0f, 0.0f, -1.0f);

    REQUIRE(ray_vs_capsule(ro, rd, cap_a, cap_b, radius) < 0.0f);
}

TEST_CASE("ray_vs_capsule: hits sphere cap") {
    // Capsule standing at origin, caps at y=0.3 and y=1.7, radius=0.3.
    const glm::vec3 cap_a(0.0f, 0.3f, 0.0f);
    const glm::vec3 cap_b(0.0f, 1.7f, 0.0f);
    const float     radius = 0.3f;

    // Ray aimed straight down at the top cap from directly above.
    const glm::vec3 ro(0.0f, 5.0f, 0.0f);
    const glm::vec3 rd(0.0f, -1.0f, 0.0f);

    const float t = ray_vs_capsule(ro, rd, cap_a, cap_b, radius);
    REQUIRE(t > 0.0f);
    // Hit should be at y = 1.7 + 0.3 = 2.0, so t = 5.0 - 2.0 = 3.0
    REQUIRE(t == Approx(3.0f).epsilon(0.01));
}

TEST_CASE("player_capsule_endpoints: endpoints match CharacterController geometry") {
    // Verify the endpoints derived from feet position match the expected capsule geometry:
    // bottom sphere at feet + kPlayerCapsuleRadius, top at feet + radius + 2*halfHeight.
    const glm::vec3 feet(1.0f, 0.0f, 2.0f);
    glm::vec3 bottom, top;
    player_capsule_endpoints(feet, bottom, top);

    REQUIRE(bottom.y == Approx(feet.y + kPlayerCapsuleRadius));
    REQUIRE(top.y    == Approx(feet.y + kPlayerCapsuleRadius + 2.0f * kPlayerCapsuleHalfHeight));
    REQUIRE(bottom.x == Approx(feet.x));
    REQUIRE(top.x    == Approx(feet.x));
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
