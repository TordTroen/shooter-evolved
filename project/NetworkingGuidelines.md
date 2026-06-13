# Networking Guidelines

Best practices for all network code in `src/net/` and any state/gameplay code that
touches replication. These are **rules**, not background — if a change violates one,
either fix the change or document why in a code comment.

Companion to [MultiplayerDesign.md](MultiplayerDesign.md): that doc is the *design*
(what we build); this doc is the *discipline* (how we write it). When the two conflict,
the design doc wins for architecture decisions, this doc wins for coding practice.

Naming, `const`, and brace rules from [CLAUDE.md](CLAUDE.md) apply here too — this doc
only adds networking-specific rules.

---

## 1. The server is the only source of truth

- **All gameplay decisions happen on the server.** Hits, damage, pickups, scores,
  authoritative position — server only. Clients send *intent*, never *outcomes*.
- **No shortcut paths for the listen-server host.** The host's local player goes through
  the exact same `input → server → state` pipeline as a remote client. A code path that
  only runs on the host is a bug waiting to repro only on dedicated. If you write
  `if (isHost)` around gameplay, stop and reconsider.
- **Clients may predict cosmetics, never consequences.** Muzzle flash, hitmarker, local
  movement — fine to show before the server confirms. Damage numbers, kills, health
  changes — only when the snapshot says so.
- **Mark every trust boundary.** Anywhere the server consumes client-supplied data, or a
  client renders an unconfirmed outcome, leave a `// CHEAT:` comment. V1 doesn't validate;
  V2 tightens these without restructuring. An unmarked boundary is an invisible boundary.

## 2. Never trust the wire

Treat every byte from a remote peer as hostile, even in insecure LAN V1.

- **Clamp before you allocate.** Lengths, counts, indices read from a `BitStream` get
  bounds-checked *before* they drive a loop, a resize, or an index. `readString()` must
  cap length; a `playerCount` must be `<= kMaxPlayers`; a `NetworkId` must resolve to a
  known entity or the message is dropped.
- **Reject, don't crash.** Malformed or out-of-range data drops the message (and ideally
  the connection), never asserts or reads out of bounds. A peer must not be able to crash
  the server with a crafted packet.
- **Validate inputs server-side regardless of prediction.** The client predicting its own
  movement does not excuse the server from re-simulating authoritatively. Prediction is a
  display optimization, not a substitute for authority.

## 3. Pick the right channel for every message

Two channels (see `Transport` / `Channel`). Choosing wrong is the most common networking bug.

| Use **Reliable-ordered** for | Use **Unreliable** for |
|---|---|
| Lobby state, scene loads | Input frames (client→server) |
| Connect / disconnect | State snapshots (server→client) |
| One-shot game events that must not be lost | Anything where "latest wins" |

- **Default to unreliable for high-frequency state.** Inputs and snapshots are sent
  continuously; a dropped one is healed by the next. Retransmitting stale state is worse
  than skipping it — it arrives late *and* delays what's behind it (head-of-line blocking).
- **Reliable is for things you'd resend by hand.** If losing the message silently breaks
  game logic (a player never spawns, a round never starts), it's reliable.
- **Redundancy over retransmit for inputs.** Include the last ~4 input frames in each
  packet rather than reliably resending one. Cheap, and self-heals a single drop with zero
  added latency.

## 4. Serialization: one code path, always round-trip tested

- **One `serialize()` function per type, handling both directions.** `BitStream` tracks
  read vs. write mode; the same function body serializes and deserializes. Never write
  separate read and write functions — they drift, and the drift is silent until it
  corrupts state on the wire.
- **Every replicated type has a round-trip test.** `write(x) → read() == x`. This is a
  hard requirement, not a nicety — a new field in `PlayerState`, `InputFrame`, or a new
  message type does not land without its round-trip test. A field added to the struct but
  forgotten in `serialize()` is a bug only the test catches.
- **Keep `serialize()` next to its type.** Free function declared in the type's header,
  defined in its `.cpp`. It travels with the struct so the two never drift apart.
- **Bit-pack deliberately, but correctness first.** Use `writeBits(value, n)` with the
  minimum `n` that fits the range, but only after the round-trip test passes. A field that
  saves 2 bits but loses the top of its range is not an optimization.
- **Version your wire format when it changes.** V1 assumes matched client/server builds.
  Any change to a message layout is a breaking change — bump the format and reject
  mismatched peers rather than silently misreading.

## 5. Keep simulation pure and deterministic

- **The server tick is a pure function: `(prior state, inputs) → new state`.** No reads of
  wall-clock time, no `rand()` without a seeded/replicated stream, no hidden globals inside
  the simulation step. This is what makes prediction, reconciliation, and lag-comp possible
  *and* testable.
- **Fixed timestep for simulation.** Server simulates at `kTickRate` (60 Hz) on a fixed
  `dt`. Accumulate real time and step in fixed increments; never feed a variable frame
  `dt` into gameplay physics.
- **Decouple render from simulation.** Rendering stays uncapped and interpolates; it never
  drives a simulation tick. Input *sampling* may run per render frame, but it produces an
  `InputFrame` consumed by the fixed-rate sim.
- **Same controller, three callers.** `CharacterController::simulate(dt, InputFrame)` is
  invoked identically by local prediction, server authority, and tests. If a caller needs a
  special variant, the abstraction is wrong.

## 6. Time, ticks, and prediction

The history buffers are trivial; the **time arithmetic is where bugs live**. Treat every
tick conversion as suspect until a unit test pins it.

- **Tag data with the tick it belongs to.** Inputs carry their client tick; snapshots
  carry their server tick. Never reason about "now" — reason about a numbered tick.
- **Reconciliation replays, it doesn't snap.** On a mismatch between predicted and
  authoritative state, rewind to the acknowledged tick and replay buffered inputs forward —
  don't teleport the player to the server position. (Level 2 milestone; the
  `(tick, InputFrame, predictedState)` ring buffer exists from Level 1 so there's no rewrite.)
- **Interpolate remote players, don't extrapolate.** Render remote entities
  `kInterpDelayTicks` behind the latest snapshot and lerp between the two straddling
  snapshots. No extrapolation in V1 — it adds artifacts and hard-to-reason-about edge cases.
- **Lag-comp arithmetic gets its own tests.** The `serverTickWhenFired` estimate, the
  rewind/restore of the history buffer — unit-test these explicitly with known inputs and
  expected outputs. A subtle off-by-one here silently makes hits feel wrong.

## 7. Drain messages at one point, own the lifetime correctly

- **One pump site.** Messages are polled and dispatched in `Net::pump()`, called once per
  game loop between event polling and state update. States never poll the transport
  directly — they read already-applied state.
- **The net layer outlives any single `GameState`.** The connection persists across
  lobby→playing transitions, so `Net` is owned by `Game`, not by a state. Never tie a
  connection's lifetime to a state's lifetime.
- **GNS types never leak past `GnsTransport.cpp`.** All transport-specific types stay
  behind the `Transport` interface. Calling code speaks `ConnectionId`, `Channel`,
  `IncomingMessage` — never a GNS type. This is what lets `MockTransport` substitute in tests.
- **Handle disconnects as a normal event, not an error.** A peer vanishing mid-game is
  expected. Clean up its `PlayerData`, free its `NetworkId` mapping, and continue — don't
  assert or tear down the session. (Full reconnect is V2; clean *disconnect* is not.)

## 8. NetworkIds and entity mapping

- **The server assigns every `NetworkId`; clients only map them.** IDs are stable for the
  entity's lifetime and never reused within a session. Clients keep a `NetworkId → Actor*`
  table and treat an unknown ID as "drop the message," never "create silently."
- **Strongly-typed, never a raw `uint32_t` in interfaces.** Pass `NetworkId`, not its
  underlying integer, so an entity id can't be confused with a tick, a connection, or an
  array index.

## 9. Testing is part of the feature, not a follow-up

Three independently testable layers (see MultiplayerDesign §Testing). A network feature
is not done until its layer is covered:

1. **Serialization** — round-trip test per replicated type. Mandatory.
2. **Simulation** — feed input sequences to the pure tick function, assert resulting state.
3. **End-to-end** — `MockTransport` wires N `Client`s to a `Server` in one process, fully
   deterministic, no real sockets. Reproduces multiplayer bugs in a unit test.

- **Reach for `MockTransport` before real sockets.** Most multiplayer logic bugs reproduce
  deterministically in-process. Real-socket testing is for transport/integration concerns,
  not gameplay logic.
- **A bug fix lands with the failing test that proves it.** Especially for time/tick
  arithmetic — the regression test is the deliverable, not an extra.

## 10. Default to small, measured, and boring

- **Bandwidth is a budget — know your numbers.** V1's 4×~30 B×20 Hz ≈ 2.5 KB/s/client is
  trivial, which is *why* full snapshots are fine. Before adding to a snapshot, do the same
  back-of-envelope. Delta compression is a deliberate V2 task, not a default.
- **Don't pre-build for scale you don't have.** No delta compression, no lobby browser, no
  reconnect, no anti-cheat validation in V1 — they're explicitly deferred. Build the V1
  scope cleanly so V2 extends it; don't pay for V2 flexibility now.
- **Latency is the enemy, not bandwidth.** Optimize for fewer round-trips and lower
  end-to-end delay before optimizing byte counts. A redundant input packet that avoids a
  reliable retransmit is the right trade.
```