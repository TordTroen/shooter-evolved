#pragma once

#include "WeaponDef.h"
#include "WeaponRuntime.h"

#include <array>
#include <optional>

namespace weapons
{
    constexpr int kMaxWeapons = 2;

    struct InventorySlot
    {
        WeaponId      id       = WeaponId::Count; // Count = empty sentinel
        WeaponRuntime runtime;
        bool          occupied = false;
    };

    enum class PickupResult
    {
        ToppedUpReserve,   // already owned this weapon - carried ammo added to reserve
        EquippedNewSlot,   // placed into a free slot and equipped
        DroppedAndEquipped // inventory was full - dropped the selected weapon first
    };

    // Per-player weapon inventory: up to kMaxWeapons slots, each with its own independent
    // WeaponRuntime (ammo/reload state). Pure - no Server/Scene dependency, unit-testable.
    class Inventory
    {
    public:
        [[nodiscard]] WeaponId equipped_weapon() const { return m_slots[m_selectedSlot].id; }
        [[nodiscard]] WeaponRuntime&       runtime()       { return m_slots[m_selectedSlot].runtime; }
        [[nodiscard]] const WeaponRuntime& runtime() const { return m_slots[m_selectedSlot].runtime; }
        [[nodiscard]] bool has(WeaponId id) const;
        [[nodiscard]] int  selected_slot() const { return m_selectedSlot; }
        [[nodiscard]] const InventorySlot& slot(int index) const { return m_slots[index]; }

        // Advances selectedSlot to the next occupied slot (wraps). No-op with zero or one
        // occupied slot.
        void switch_next();

        // id/ammoInMag/reserveAmmo describe the weapon item being picked up (its carried
        // ammo); def is that weapon's definition, used to clamp reserve top-ups.
        // droppedWeapon is set only when the result is DroppedAndEquipped. droppedRuntimeOut,
        // if non-null, receives the dropped weapon's runtime (ammo/reload state) so the
        // caller can spawn a floor item carrying its actual ammo instead of a fresh one.
        PickupResult add_or_pickup(WeaponId id, int ammoInMag, int reserveAmmo,
                                    const WeaponDef& def, std::optional<WeaponId>& droppedWeapon,
                                    WeaponRuntime* droppedRuntimeOut = nullptr);

        // Clears the selected slot and moves selection to a remaining occupied slot.
        // No-op (returns false) when only one slot is occupied - a player always holds at
        // least one weapon (decision #1).
        bool drop_selected(WeaponId& droppedId, WeaponRuntime& droppedRuntime);

    private:
        std::array<InventorySlot, kMaxWeapons> m_slots;
        int                                     m_selectedSlot = 0;

        [[nodiscard]] int occupied_count() const;
        [[nodiscard]] int first_free_slot() const; // -1 if none
    };

    // Starting loadout: kDefaultWeapon in slot 0, everything else empty, slot 0 selected.
    Inventory make_starting_inventory();
}
