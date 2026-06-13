#include "FireIntent.h"
#include "BitStream.h"

void serialize(BitStream& bs, FireIntent& fi)
{
    bs.serializeBits(fi.shooterId.value, 32);
    bs.serializeBits(fi.clientTick, 32);
    bs.serializeVec3(fi.origin);
    bs.serializeVec3(fi.direction);
}
