#pragma once

#include "NetworkId.h"

#include <cstddef>
#include <string>
#include <vector>

class BitStream;

struct RosterEntry
{
    NetworkId   netId;
    std::string name;
};

// Full server→client lobby roster (see NetworkingGuidelines §4). One bidirectional
// serialize() handles both read and write. Player count is clamped before driving
// its loop; an out-of-range count marks the stream in error and the caller drops it
// (NetworkingGuidelines §2).
struct LobbyRoster
{
    static constexpr int    MAX_PLAYERS  = 4;  // mirrors Server::kMaxPlayers
    static constexpr size_t MAX_NAME_LEN = 24; // wire cap; clamp on read

    std::vector<RosterEntry> players; // capped at MAX_PLAYERS on read
};

void serialize(BitStream& bs, LobbyRoster& roster); // one bidirectional path (§4)
