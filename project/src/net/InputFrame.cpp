#include "InputFrame.h"
#include "BitStream.h"

void serialize(BitStream& bs, InputFrame& f)
{
    bs.serializeFloat(f.move.x);
    bs.serializeFloat(f.move.y);
    bs.serializeFloat(f.yaw);
    bs.serializeFloat(f.pitch);
    bs.serializeBits(f.buttons, 8);
    bs.serializeBits(f.tick, 32);
}
