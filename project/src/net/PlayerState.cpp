#include "PlayerState.h"
#include "BitStream.h"

void serialize(BitStream& bs, PlayerState& ps)
{
    bs.serializeVec3(ps.position);
    bs.serializeFloat(ps.yaw);
    bs.serializeFloat(ps.pitch);

    // Health stored as signed 16-bit (max ~32k — sufficient for V1).
    auto health = static_cast<uint32_t>(static_cast<uint16_t>(ps.health));
    bs.serializeBits(health, 16);
    ps.health = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(health)));

    bs.serializeBits(ps.buttons, 8);
    bs.serializeBits(ps.lastProcessedInputTick, 32);
    bs.serializeBits(ps.fireCount, 32);

    uint32_t alive = ps.isAlive ? 1u : 0u;
    bs.serializeBits(alive, 1);
    if (bs.isReading())
        ps.isAlive = (alive != 0u);

    bs.serializeFloat(ps.respawnRemaining);

    // uint16_t counters go through the uint32_t overload of serializeBits, same
    // truncate/widen pattern as health above.
    auto kills = static_cast<uint32_t>(ps.kills);
    bs.serializeBits(kills, 16);
    ps.kills = static_cast<uint16_t>(kills);

    auto deaths = static_cast<uint32_t>(ps.deaths);
    bs.serializeBits(deaths, 16);
    ps.deaths = static_cast<uint16_t>(deaths);
}
