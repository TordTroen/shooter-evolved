#include "WeaponItemState.h"
#include "BitStream.h"

void serialize(BitStream& bs, WeaponItemState& item)
{
    bs.serializeBits(item.netId.value, 32);

    auto weapon = static_cast<uint32_t>(item.weapon);
    bs.serializeBits(weapon, 8);
    if (bs.isReading())
        item.weapon = static_cast<weapons::WeaponId>(static_cast<uint8_t>(weapon));

    bs.serializeVec3(item.position);

    // Quaternion: 4 floats (correctness first; bit-pack in V2), matches ActorState.
    bs.serializeFloat(item.rotation.x);
    bs.serializeFloat(item.rotation.y);
    bs.serializeFloat(item.rotation.z);
    bs.serializeFloat(item.rotation.w);

    // Ammo as signed 16-bit, matches PlayerState::ammoInMag/reserveAmmo.
    auto ammoInMag = static_cast<uint32_t>(static_cast<uint16_t>(item.ammoInMag));
    bs.serializeBits(ammoInMag, 16);
    if (bs.isReading())
        item.ammoInMag = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(ammoInMag)));

    auto reserveAmmo = static_cast<uint32_t>(static_cast<uint16_t>(item.reserveAmmo));
    bs.serializeBits(reserveAmmo, 16);
    if (bs.isReading())
        item.reserveAmmo = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(reserveAmmo)));

    // isAlive: 1 bit.
    uint32_t alive = item.isAlive ? 1u : 0u;
    bs.serializeBits(alive, 1);
    if (bs.isReading())
        item.isAlive = (alive != 0u);
}
