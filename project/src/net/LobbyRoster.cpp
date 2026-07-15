#include "LobbyRoster.h"
#include "BitStream.h"

void serialize(BitStream& bs, LobbyRoster& roster)
{
    uint32_t count = static_cast<uint32_t>(roster.players.size());
    bs.serializeBits(count, 3); // 3 bits covers 0-4 players

    if (bs.isReading())
    {
        // Clamp before allocating/looping — never trust the wire (§2).
        if (count > static_cast<uint32_t>(LobbyRoster::MAX_PLAYERS))
        {
            bs.markError();
            return;
        }
        roster.players.resize(count);
    }

    for (auto& entry : roster.players)
    {
        bs.serializeBits(entry.netId.value, 32);
        bs.serializeString(entry.name);

        if (bs.isReading() && entry.name.size() > LobbyRoster::MAX_NAME_LEN)
        {
            // Names are cosmetic — truncation is friendlier than dropping the
            // whole roster. readString() already clamps to the remaining
            // buffer; this is a semantic cap on top of that safety cap.
            entry.name.resize(LobbyRoster::MAX_NAME_LEN);
        }
    }

    bs.serializeBits(roster.leaderId.value, 32);
}
