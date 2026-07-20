#include "Snapshot.h"
#include "BitStream.h"

void serialize(BitStream& bs, SnapshotMessage& snap)
{
    bs.serializeBits(snap.serverTick, 32);

    uint32_t count = static_cast<uint32_t>(snap.players.size());
    bs.serializeBits(count, 8);

    if (bs.isReading())
    {
        // Clamp before allocating/looping - never trust the wire.
        if (count > static_cast<uint32_t>(SnapshotMessage::kMaxPlayers))
        {
            bs.markError();
            return;
        }
        snap.players.resize(count);
    }

    for (auto& entry : snap.players)
    {
        bs.serializeBits(entry.netId.value, 32);
        serialize(bs, entry.state);
    }

    // Actor section: replicated scene props / targets.
    uint32_t actorCount = static_cast<uint32_t>(snap.actors.size());
    bs.serializeBits(actorCount, 8);

    if (bs.isReading())
    {
        // Clamp before allocating/looping - never trust the wire (§2).
        if (actorCount > static_cast<uint32_t>(SnapshotMessage::kMaxActors))
        {
            bs.markError();
            return;
        }
        snap.actors.resize(actorCount);
    }

    for (auto& as : snap.actors)
        serialize(bs, as);

    // Weapon item section: floor pickups + dropped weapons.
    uint32_t weaponItemCount = static_cast<uint32_t>(snap.weaponItems.size());
    bs.serializeBits(weaponItemCount, 8);

    if (bs.isReading())
    {
        // Clamp before allocating/looping - never trust the wire (§2).
        if (weaponItemCount > static_cast<uint32_t>(SnapshotMessage::kMaxWeaponItems))
        {
            bs.markError();
            return;
        }
        snap.weaponItems.resize(weaponItemCount);
    }

    for (auto& item : snap.weaponItems)
        serialize(bs, item);
}
