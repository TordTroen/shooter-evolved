#pragma once

#include "ActorState.h"
#include "NetworkId.h"
#include "PlayerState.h"
#include "WeaponItemState.h"

#include <cstdint>
#include <vector>

class BitStream;

// Full server→client state snapshot (see MultiplayerDesign §replication). One
// bidirectional serialize() handles both read and write so the two never drift.
// Player and actor counts are clamped before driving their loops; out-of-range
// counts mark the stream in error and the caller drops it (NetworkingGuidelines §2).
struct SnapshotMessage
{
    static constexpr int kMaxPlayers     = 4;  // mirrors Server::kMaxPlayers
    static constexpr int kMaxActors      = 16; // scene props / targets
    static constexpr int kMaxWeaponItems = 16; // floor pickups + dropped weapons

    struct Entry
    {
        NetworkId   netId;
        PlayerState state;
    };

    uint32_t                     serverTick = 0;
    std::vector<Entry>           players;     // capped at kMaxPlayers on read
    std::vector<ActorState>      actors;      // capped at kMaxActors on read
    std::vector<WeaponItemState> weaponItems; // capped at kMaxWeaponItems on read
};

void serialize(BitStream& bs, SnapshotMessage& snap);
