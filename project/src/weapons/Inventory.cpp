#include "Inventory.h"
#include "WeaponRegistry.h"

namespace weapons
{
    bool Inventory::has(WeaponId id) const
    {
        for (const auto& s : m_slots)
        {
            if (s.occupied && s.id == id) { return true; }
        }
        return false;
    }

    int Inventory::occupied_count() const
    {
        int count = 0;
        for (const auto& s : m_slots)
        {
            if (s.occupied) { ++count; }
        }
        return count;
    }

    int Inventory::first_free_slot() const
    {
        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            if (!m_slots[i].occupied) { return static_cast<int>(i); }
        }
        return -1;
    }

    void Inventory::switch_next()
    {
        if (occupied_count() <= 1) { return; }

        int idx = m_selectedSlot;
        do
        {
            idx = (idx + 1) % static_cast<int>(m_slots.size());
        } while (!m_slots[idx].occupied);
        m_selectedSlot = idx;
    }

    PickupResult Inventory::add_or_pickup(WeaponId id, int ammoInMag, int reserveAmmo,
                                           const WeaponDef& def, std::optional<WeaponId>& droppedWeapon,
                                           WeaponRuntime* droppedRuntimeOut)
    {
        droppedWeapon.reset();

        if (has(id))
        {
            for (auto& s : m_slots)
            {
                if (s.occupied && s.id == id)
                {
                    s.runtime.add_reserve(ammoInMag + reserveAmmo, def);
                    break;
                }
            }
            return PickupResult::ToppedUpReserve;
        }

        int freeSlot = first_free_slot();
        PickupResult result = PickupResult::EquippedNewSlot;

        if (freeSlot < 0)
        {
            // Inventory full - drop the selected weapon first, then take its freed slot.
            WeaponId      dropped;
            WeaponRuntime droppedRuntime;
            drop_selected(dropped, droppedRuntime); // always succeeds: full implies >1 occupied
            droppedWeapon = dropped;
            if (droppedRuntimeOut) { *droppedRuntimeOut = droppedRuntime; }
            freeSlot      = first_free_slot();
            result        = PickupResult::DroppedAndEquipped;
        }

        InventorySlot& slot = m_slots[freeSlot];
        slot.id             = id;
        slot.occupied        = true;
        slot.runtime         = WeaponRuntime{};
        slot.runtime.ammoInMag   = ammoInMag;
        slot.runtime.reserveAmmo = reserveAmmo;
        m_selectedSlot = freeSlot;

        return result;
    }

    bool Inventory::drop_selected(WeaponId& droppedId, WeaponRuntime& droppedRuntime)
    {
        if (occupied_count() <= 1) { return false; }

        InventorySlot& selected = m_slots[m_selectedSlot];
        droppedId                = selected.id;
        droppedRuntime           = selected.runtime;
        selected.occupied        = false;
        selected.id              = WeaponId::Count;
        selected.runtime         = WeaponRuntime{};

        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            if (m_slots[i].occupied)
            {
                m_selectedSlot = static_cast<int>(i);
                break;
            }
        }
        return true;
    }

    Inventory make_starting_inventory()
    {
        const WeaponDef& def = registry().def(kDefaultWeapon);

        Inventory inv;
        std::optional<WeaponId> dropped;
        inv.add_or_pickup(kDefaultWeapon, def.magazineCapacity, def.reserveAmmoMax, def, dropped);
        return inv;
    }
}
