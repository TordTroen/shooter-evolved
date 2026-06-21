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
}
