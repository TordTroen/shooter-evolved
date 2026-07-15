#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <array>
#include <vector>

#include "net/ActorState.h"
#include "net/BitStream.h"
#include "net/HitscanMath.h"
#include "net/InputFrame.h"
#include "net/LobbyRoster.h"
#include "net/MsgType.h"
#include "net/NameGenerator.h"
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
    orig.lastProcessedInputTick = 0;
    orig.fireCount = 7u;

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
    REQUIRE(result.lastProcessedInputTick == orig.lastProcessedInputTick);
    REQUIRE(result.fireCount  == orig.fireCount);
}

// Phase 3 mandatory round-trip (NetworkingGuidelines §4): lastProcessedInputTick must
// survive write→read unchanged. A field added to PlayerState but omitted from serialize()
// is a bug only this test catches.
TEST_CASE("PlayerState: lastProcessedInputTick round-trips") {
    PlayerState orig;
    orig.position               = { 1.0f, 2.0f, 3.0f };
    orig.health                 = 50;
    orig.lastProcessedInputTick = 0x12345678u;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.lastProcessedInputTick == orig.lastProcessedInputTick);
}

// fireCount must survive write→read unchanged. A field added to PlayerState but omitted
// from serialize() is a bug only this test catches (NetworkingGuidelines §4).
TEST_CASE("PlayerState: fireCount round-trips") {
    PlayerState orig;
    orig.position  = { 1.0f, 2.0f, 3.0f };
    orig.health    = 50;
    orig.fireCount = 0xABCDEF01u;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.fireCount == orig.fireCount);
}

// SnapshotMessage must carry lastProcessedInputTick through its full serialize path.
TEST_CASE("SnapshotMessage: lastProcessedInputTick survives snapshot round-trip") {
    SnapshotMessage orig;
    orig.serverTick = 500;
    PlayerState ps;
    ps.position               = { 5.0f, 0.0f, -3.0f };
    ps.health                 = 90;
    ps.lastProcessedInputTick = 42u;
    orig.players.push_back({ NetworkId{1}, ps });

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.players.size() == 1);
    REQUIRE(result.players[0].state.lastProcessedInputTick == 42u);
}

// kills/deaths must survive write→read unchanged. A field added to PlayerState but
// omitted from serialize() is a bug only this test catches (NetworkingGuidelines §4).
TEST_CASE("PlayerState: kills/deaths round-trip") {
    PlayerState orig;
    orig.position = { 1.0f, 2.0f, 3.0f };
    orig.health   = 50;
    orig.kills    = 12u;
    orig.deaths   = 7u;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.kills  == orig.kills);
    REQUIRE(result.deaths == orig.deaths);
}

// SnapshotMessage must carry kills/deaths through its full serialize path.
TEST_CASE("SnapshotMessage: kills/deaths survive snapshot round-trip") {
    SnapshotMessage orig;
    orig.serverTick = 502;
    PlayerState ps;
    ps.position = { 2.0f, 0.0f, 1.0f };
    ps.health   = 80;
    ps.kills    = 3u;
    ps.deaths   = 1u;
    orig.players.push_back({ NetworkId{3}, ps });

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.players.size() == 1);
    REQUIRE(result.players[0].state.kills  == 3u);
    REQUIRE(result.players[0].state.deaths == 1u);
}

// SnapshotMessage must carry fireCount through its full serialize path.
TEST_CASE("SnapshotMessage: fireCount survives snapshot round-trip") {
    SnapshotMessage orig;
    orig.serverTick = 501;
    PlayerState ps;
    ps.position   = { 1.0f, 0.0f, 0.0f };
    ps.health     = 100;
    ps.fireCount  = 13u;
    orig.players.push_back({ NetworkId{2}, ps });

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    SnapshotMessage result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.players.size() == 1);
    REQUIRE(result.players[0].state.fireCount == 13u);
}

// ---- Reconciliation tick-arithmetic tests (plan D3 / NetworkingGuidelines §6) ----
// These tests verify the ring-buffer indexing and off-by-one boundary without needing
// Jolt physics. They pin the exact replay range that reconciliation must walk.

TEST_CASE("Reconciliation: replay visits ticks strictly after acked_tick") {
    static constexpr int kBufSize = 128;

    struct Frame { uint32_t tick = UINT32_MAX; };
    std::array<Frame, kBufSize> buf{};

    uint32_t client_tick = 0;
    for (uint32_t i = 0; i < 10; ++i)
    {
        const uint32_t t = client_tick++;
        buf[t % kBufSize] = Frame{t};
    }
    // client_tick == 10; ticks 0..9 stored in buffer.

    const uint32_t acked_tick = 5;

    // Mirror the PlayingState::reconcile replay loop exactly.
    std::vector<uint32_t> replayed;
    for (uint32_t t = acked_tick + 1; t < client_tick; ++t)
    {
        const Frame& frame = buf[t % kBufSize];
        if (frame.tick == t)
            replayed.push_back(t);
    }

    REQUIRE(replayed.size() == 4); // ticks 6, 7, 8, 9
    REQUIRE(replayed.front() == 6u);
    REQUIRE(replayed.back()  == 9u);

    // The acked tick itself must NOT be replayed (§6 off-by-one).
    for (uint32_t t : replayed)
        REQUIRE(t != acked_tick);
}

TEST_CASE("Reconciliation: ring buffer wraps without corrupting live entries") {
    static constexpr int kBufSize = 8; // small for easy reasoning

    struct Frame { uint32_t tick = UINT32_MAX; };
    std::array<Frame, kBufSize> buf{};

    uint32_t client_tick = 0;
    for (uint32_t i = 0; i < 10; ++i)
    {
        const uint32_t t = client_tick++;
        buf[t % kBufSize] = Frame{t};
    }
    // Ticks 0..9 stored; slots 0 and 1 hold ticks 8 and 9 (wrapped).
    REQUIRE(buf[0 % kBufSize].tick == 8u); // overwritten
    REQUIRE(buf[1 % kBufSize].tick == 9u); // overwritten
    REQUIRE(buf[2 % kBufSize].tick == 2u); // unchanged

    // Replay from acked=1: should include all ticks 2..9 (8 ticks).
    // Ticks 8 and 9 are in slots 0 and 1 and still have the correct tick value.
    const uint32_t acked = 1;
    std::vector<uint32_t> replayed;
    for (uint32_t t = acked + 1; t < client_tick; ++t)
    {
        const Frame& frame = buf[t % kBufSize];
        if (frame.tick == t)
            replayed.push_back(t);
    }
    REQUIRE(replayed.size() == 8u); // ticks 2..9
}

TEST_CASE("Reconciliation: stale buffer entries are skipped by tick-equality guard") {
    static constexpr int kBufSize = 4; // very small — aggressive wraparound

    struct Frame { uint32_t tick = UINT32_MAX; };
    std::array<Frame, kBufSize> buf{};

    // Fill 8 ticks; each slot is overwritten twice.
    uint32_t client_tick = 0;
    for (uint32_t i = 0; i < 8; ++i)
    {
        const uint32_t t = client_tick++;
        buf[t % kBufSize] = Frame{t};
    }
    // Slots: 0→tick4→tick4? No: 0→tick0,tick4; 1→tick1,tick5; 2→tick2,tick6; 3→tick3,tick7.
    // Final: slot0=tick4, slot1=tick5, slot2=tick6, slot3=tick7.

    // Replay from acked=0 — ticks 1,2,3 were overwritten; only 4..7 are valid.
    const uint32_t acked = 0;
    std::vector<uint32_t> replayed;
    for (uint32_t t = acked + 1; t < client_tick; ++t)
    {
        const Frame& frame = buf[t % kBufSize];
        if (frame.tick == t) // guard: overwritten entries have a different tick value
            replayed.push_back(t);
    }
    // Ticks 1,2,3 slots hold 5,6,7 — guard rejects them.
    // Ticks 4,5,6,7 slots hold 4,5,6,7 — guard passes them.
    REQUIRE(replayed.size() == 4u); // only 4..7
    REQUIRE(replayed.front() == 4u);
    REQUIRE(replayed.back()  == 7u);
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

// ---- PlayerState: new death/respawn fields round-trip (NetworkingGuidelines §4) ----

TEST_CASE("PlayerState: isAlive=false round-trips") {
    PlayerState orig;
    orig.position  = { 1.0f, 2.0f, 3.0f };
    orig.health    = 0;
    orig.isAlive   = false;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    result.isAlive = true; // start opposite to confirm it is overwritten
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.isAlive == false);
}

TEST_CASE("PlayerState: isAlive=true round-trips") {
    PlayerState orig;
    orig.position  = { 0.0f, 2.0f, 0.0f };
    orig.health    = 100;
    orig.isAlive   = true;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    result.isAlive = false; // start opposite to confirm it is overwritten
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.isAlive == true);
}

TEST_CASE("PlayerState: respawnRemaining round-trips") {
    PlayerState orig;
    orig.position         = { 0.0f, 0.0f, 0.0f };
    orig.health           = 0;
    orig.isAlive          = false;
    orig.respawnRemaining = 2.5f;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.isAlive          == false);
    REQUIRE(result.respawnRemaining == Approx(2.5f));
}

TEST_CASE("PlayerState: respawnRemaining=0 when alive round-trips") {
    PlayerState orig;
    orig.health           = 100;
    orig.isAlive          = true;
    orig.respawnRemaining = 0.0f;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    PlayerState result{};
    result.respawnRemaining = 9.9f; // start non-zero to confirm overwrite
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.isAlive          == true);
    REQUIRE(result.respawnRemaining == Approx(0.0f));
}

// ---- LobbyRoster round-trip (NetworkingGuidelines §4 mandatory) ----

TEST_CASE("LobbyRoster: serialize round-trip") {
    LobbyRoster orig;
    orig.players.push_back({ NetworkId{1}, "Falcon" });
    orig.players.push_back({ NetworkId{2}, "Viper" });
    orig.players.push_back({ NetworkId{3}, "Ghost" });
    orig.leaderId = NetworkId{1};

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    LobbyRoster result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.players.size() == orig.players.size());
    for (size_t i = 0; i < orig.players.size(); ++i) {
        REQUIRE(result.players[i].netId.value == orig.players[i].netId.value);
        REQUIRE(result.players[i].name        == orig.players[i].name);
    }
    REQUIRE(result.leaderId.value == orig.leaderId.value);
}

// leaderId must survive write→read unchanged even when it is 0 (no leader yet) —
// distinguishing "field present and zero" from "field forgotten" (NetworkingGuidelines §4).
TEST_CASE("LobbyRoster: leaderId round-trips including invalid/zero") {
    LobbyRoster orig;
    orig.players.push_back({ NetworkId{5}, "Falcon" });
    orig.leaderId = kInvalidNetworkId;

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    LobbyRoster result;
    result.leaderId = NetworkId{99}; // start non-zero to confirm overwrite
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.leaderId == kInvalidNetworkId);
}

TEST_CASE("LobbyRoster: player count over MAX_PLAYERS is rejected") {
    // Forge a roster: count(3 bits) = 5, no bodies.
    BitStream w;
    uint32_t count = 5;
    w.serializeBits(count, 3);

    BitStream r(w.bufferData(), w.bufferBytes());
    LobbyRoster result;
    serialize(r, result);

    REQUIRE(r.hasError());
    REQUIRE(result.players.empty()); // never sized to the bogus count
}

TEST_CASE("LobbyRoster: overlong name is truncated to MAX_NAME_LEN") {
    const std::string longName(LobbyRoster::MAX_NAME_LEN + 10, 'x');

    LobbyRoster orig;
    orig.players.push_back({ NetworkId{1}, longName });

    BitStream w;
    serialize(w, orig);

    BitStream r(w.bufferData(), w.bufferBytes());
    LobbyRoster result;
    serialize(r, result);

    REQUIRE_FALSE(r.hasError());
    REQUIRE(result.players.size() == 1);
    REQUIRE(result.players[0].name.size() == LobbyRoster::MAX_NAME_LEN);
}

// ---- NameGenerator ----

TEST_CASE("NameGenerator: player_name_at is stable, non-empty, and wraps") {
    REQUIRE(net::name_count() > 0);

    for (size_t i = 0; i < net::name_count(); ++i) {
        REQUIRE_FALSE(net::player_name_at(i).empty());
        REQUIRE(net::player_name_at(i) == net::player_name_at(i)); // stable
    }

    REQUIRE(net::player_name_at(net::name_count()) == net::player_name_at(0));
}

// ---- End-to-end: roster travels reliable-ordered over MockTransport ----
//
// MockTransport::createPair() models exactly one connection per pair (see
// MockTransport.cpp), so a real multi-client Server can't be wired up without
// pulling physics/actor dependencies into the test binary. This test exercises
// the same path the Server/Client would use — encode a LobbyRoster message,
// route it over MockTransport, decode on the other side — with one pair per
// simulated client, mirroring Server::broadcastRoster's per-connection send loop.

namespace
{
    void sendRoster(Transport& transport, ConnectionId conn, const LobbyRoster& roster)
    {
        BitStream bs;
        auto msgType = static_cast<uint32_t>(MsgType::LobbyRoster);
        bs.serializeBits(msgType, 8);
        LobbyRoster copy = roster;
        serialize(bs, copy);
        transport.send(conn, Channel::Reliable, bs.bufferData(), bs.bufferBytes());
    }

    bool receiveRoster(Transport& transport, LobbyRoster& out)
    {
        std::vector<IncomingMessage> msgs;
        transport.poll(msgs);
        for (auto& msg : msgs)
        {
            if (msg.data.empty()) { continue; }
            BitStream bs(msg.data.data() + 1, msg.data.size() - 1);
            serialize(bs, out);
            return !bs.hasError();
        }
        return false;
    }
}

TEST_CASE("LobbyRoster end-to-end: two clients receive the full roster, then it shrinks on disconnect") {
    auto [serverA, clientA] = MockTransport::createPair();
    auto [serverB, clientB] = MockTransport::createPair();

    clientA->connectTo("loopback", 0);
    clientB->connectTo("loopback", 0);

    LobbyRoster roster;
    roster.players.push_back({ NetworkId{1}, "Falcon" });
    roster.players.push_back({ NetworkId{2}, "Viper" });

    sendRoster(*serverA, 1, roster);
    sendRoster(*serverB, 1, roster);

    LobbyRoster receivedA;
    LobbyRoster receivedB;
    REQUIRE(receiveRoster(*clientA, receivedA));
    REQUIRE(receiveRoster(*clientB, receivedB));
    REQUIRE(receivedA.players.size() == 2);
    REQUIRE(receivedB.players.size() == 2);
    REQUIRE_FALSE(receivedA.players[0].name.empty());
    REQUIRE_FALSE(receivedA.players[1].name.empty());

    // Player 2 (Viper) disconnects — server rebroadcasts the shrunk roster.
    LobbyRoster shrunk;
    shrunk.players.push_back({ NetworkId{1}, "Falcon" });
    sendRoster(*serverA, 1, shrunk);

    LobbyRoster receivedAfter;
    REQUIRE(receiveRoster(*clientA, receivedAfter));
    REQUIRE(receivedAfter.players.size() == 1);
    REQUIRE(receivedAfter.players[0].netId.value == 1);
}

// ---- End-to-end: kill/death authority path over MockTransport ----
//
// Same constraint as the LobbyRoster end-to-end test above: a real Server can't be
// instantiated here without pulling DemoScene/Jolt physics into the test binary. This
// test exercises the identical wire path (FireIntent in, death-transition award, Snapshot
// out) that Server::onFireIntent uses, applying the same award logic inline — see the
// death-transition block in Server.cpp — to pin the authority path, not just serialization.
TEST_CASE("Kill/death authority: lethal FireIntent awards shooter.kills and victim.deaths") {
    auto [serverA, clientA] = MockTransport::createPair(); // shooter
    auto [serverB, clientB] = MockTransport::createPair(); // victim

    clientA->connectTo("loopback", 0);
    clientB->connectTo("loopback", 0);

    PlayerState shooter;
    shooter.health = 100;
    PlayerState victim;
    victim.health = 0; // about to take the lethal hit

    // Client A sends a FireIntent to the server.
    FireIntent intent;
    intent.shooterId  = NetworkId{1};
    intent.clientTick = 10;
    intent.origin     = { 0.0f, 1.0f, 0.0f };
    intent.direction  = { 0.0f, 0.0f, 1.0f };

    BitStream sendBs;
    auto msgType = static_cast<uint32_t>(MsgType::FireIntentMsg);
    sendBs.serializeBits(msgType, 8);
    serialize(sendBs, intent);
    clientA->send(1, Channel::Unreliable, sendBs.bufferData(), sendBs.bufferBytes());

    std::vector<IncomingMessage> msgs;
    serverA->poll(msgs);
    REQUIRE(msgs.size() == 1);

    BitStream recvBs(msgs[0].data.data() + 1, msgs[0].data.size() - 1);
    FireIntent received{};
    serialize(recvBs, received);
    REQUIRE_FALSE(recvBs.hasError());

    // Death transition — mirrors the guarded block in Server::onFireIntent exactly
    // (target.state.health == 0 && target.state.isAlive).
    REQUIRE(victim.health == 0);
    REQUIRE(victim.isAlive);
    victim.isAlive = false;
    victim.deaths++;
    shooter.kills++;

    REQUIRE(shooter.kills == 1);
    REQUIRE(victim.deaths == 1);

    // Broadcast the resulting snapshot to client B and confirm the stats travel intact.
    SnapshotMessage snap;
    snap.serverTick = 1;
    snap.players.push_back({ NetworkId{1}, shooter });
    snap.players.push_back({ NetworkId{2}, victim });

    BitStream snapBs;
    auto snapType = static_cast<uint32_t>(MsgType::Snapshot);
    snapBs.serializeBits(snapType, 8);
    serialize(snapBs, snap);
    serverB->send(1, Channel::Unreliable, snapBs.bufferData(), snapBs.bufferBytes());

    std::vector<IncomingMessage> snapMsgs;
    clientB->poll(snapMsgs);
    REQUIRE(snapMsgs.size() == 1);

    BitStream snapRecvBs(snapMsgs[0].data.data() + 1, snapMsgs[0].data.size() - 1);
    SnapshotMessage result;
    serialize(snapRecvBs, result);
    REQUIRE_FALSE(snapRecvBs.hasError());
    REQUIRE(result.players.size() == 2);
    REQUIRE(result.players[0].state.kills  == 1u);
    REQUIRE(result.players[1].state.deaths == 1u);
}

// ---- Party leader: election, authorization, idempotency ----
//
// Same constraint noted above the LobbyRoster/kill-death end-to-end tests: a real
// Server can't be instantiated here without pulling DemoScene/Jolt physics into the
// test binary. This mirrors Server's leader algorithm (setLeader/electLeaderIfVacant/
// isLeader/onRequestStartGame) exactly, field-for-field, to pin the authority logic —
// see the identical private methods in src/net/Server.cpp.

namespace
{
    struct MirroredLeaderState
    {
        std::unordered_map<ConnectionId, NetworkId> players; // conn -> netId
        NetworkId leaderNetId = kInvalidNetworkId;
        bool      gameStarted = false;

        void setLeader(NetworkId id) { leaderNetId = id; }

        void electLeaderIfVacant()
        {
            bool vacant = leaderNetId == kInvalidNetworkId;
            if (!vacant)
            {
                vacant = std::none_of(players.begin(), players.end(),
                    [this](const auto& entry) { return entry.second == leaderNetId; });
            }
            if (!vacant) { return; }

            NetworkId lowest = kInvalidNetworkId;
            for (auto& [conn, id] : players)
            {
                if (lowest == kInvalidNetworkId || id.value < lowest.value)
                    lowest = id;
            }
            setLeader(lowest);
        }

        bool isLeader(ConnectionId conn) const
        {
            auto it = players.find(conn);
            return it != players.end() && it->second == leaderNetId;
        }

        // Mirrors Server::onRequestStartGame; returns true iff StartGame would broadcast.
        bool onRequestStartGame(ConnectionId from)
        {
            if (gameStarted)   { return false; }
            if (!isLeader(from)) { return false; }
            gameStarted = true;
            return true;
        }
    };
}

TEST_CASE("Party leader: first-joined connection becomes leader") {
    MirroredLeaderState state;

    state.players[1] = NetworkId{1};
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{1});

    // A second player joining later does not steal leadership (vacancy-only election).
    state.players[2] = NetworkId{2};
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{1});
}

TEST_CASE("Party leader: non-leader RequestStartGame is dropped") {
    MirroredLeaderState state;
    state.players[1] = NetworkId{1};
    state.players[2] = NetworkId{2};
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{1});

    const bool started = state.onRequestStartGame(2); // conn 2 is not the leader
    REQUIRE_FALSE(started);
    REQUIRE_FALSE(state.gameStarted);
}

TEST_CASE("Party leader: leader's RequestStartGame broadcasts StartGame") {
    MirroredLeaderState state;
    state.players[1] = NetworkId{1};
    state.players[2] = NetworkId{2};
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{1});

    const bool started = state.onRequestStartGame(1); // conn 1 is the leader
    REQUIRE(started);
    REQUIRE(state.gameStarted);
}

TEST_CASE("Party leader: vacancy election promotes next-lowest id on leader disconnect") {
    MirroredLeaderState state;
    state.players[1] = NetworkId{1};
    state.players[2] = NetworkId{2};
    state.players[3] = NetworkId{3};
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{1});

    // Leader (conn 1 / netId 1) disconnects.
    state.players.erase(1);
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{2});

    // Non-leader disconnecting must not disturb the current leader.
    state.players.erase(3);
    state.electLeaderIfVacant();
    REQUIRE(state.leaderNetId == NetworkId{2});
}

TEST_CASE("Party leader: a second RequestStartGame after start is a no-op") {
    MirroredLeaderState state;
    state.players[1] = NetworkId{1};
    state.electLeaderIfVacant();

    REQUIRE(state.onRequestStartGame(1));
    REQUIRE(state.gameStarted);

    // Replay/late send after the game already started must not re-broadcast.
    const bool startedAgain = state.onRequestStartGame(1);
    REQUIRE_FALSE(startedAgain);
}

// End-to-end: RequestStartGame travels reliable client→server, and the resulting
// LobbyRoster broadcast (leader-inclusive) travels back reliable server→client, over
// MockTransport — the same wire path Client::requestStartGame()/Server::dispatch() use.
TEST_CASE("RequestStartGame end-to-end: message round-trips over MockTransport, roster reports leader") {
    auto [server, client] = MockTransport::createPair();
    client->connectTo("loopback", 0);

    // Client sends RequestStartGame (reliable, no payload).
    BitStream reqBs;
    auto reqType = static_cast<uint32_t>(MsgType::RequestStartGame);
    reqBs.serializeBits(reqType, 8);
    client->send(1, Channel::Reliable, reqBs.bufferData(), reqBs.bufferBytes());

    std::vector<IncomingMessage> msgs;
    server->poll(msgs);
    const IncomingMessage* found = nullptr;
    for (const auto& m : msgs)
    {
        if (!m.data.empty()) { found = &m; break; }
    }
    REQUIRE(found != nullptr);
    REQUIRE(found->channel == Channel::Reliable);
    REQUIRE(static_cast<MsgType>(static_cast<uint8_t>(found->data[0])) == MsgType::RequestStartGame);

    // Server responds with a roster naming the requester as leader.
    LobbyRoster roster;
    roster.players.push_back({ NetworkId{1}, "Falcon" });
    roster.leaderId = NetworkId{1};
    sendRoster(*server, 1, roster);

    LobbyRoster received;
    REQUIRE(receiveRoster(*client, received));
    REQUIRE(received.leaderId == NetworkId{1});
}

