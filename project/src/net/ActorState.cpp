#include "ActorState.h"
#include "BitStream.h"

void serialize(BitStream& bs, ActorState& as)
{
    bs.serializeBits(as.netId.value, 32);
    bs.serializeVec3(as.position);

    // Quaternion: 4 floats (correctness first; bit-pack in V2).
    bs.serializeFloat(as.rotation.x);
    bs.serializeFloat(as.rotation.y);
    bs.serializeFloat(as.rotation.z);
    bs.serializeFloat(as.rotation.w);

    // Health as signed 16-bit (matches PlayerState - max ~32k, sufficient for V1).
    auto health = static_cast<uint32_t>(static_cast<uint16_t>(as.health));
    bs.serializeBits(health, 16);
    if (bs.isReading())
        as.health = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(health)));

    // isAlive: 1 bit.
    uint32_t alive = as.isAlive ? 1u : 0u;
    bs.serializeBits(alive, 1);
    if (bs.isReading())
        as.isAlive = (alive != 0u);
}
