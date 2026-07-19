#pragma once

#include "WeaponDef.h"

#include <array>
#include <cstddef>

namespace weapons
{
    // Data-driven weapon registry - code-registered at startup for now, can move to a
    // data file later. No engine dependencies, trivially unit-testable.
    class WeaponRegistry
    {
    public:
        WeaponRegistry();

        // Out-of-range/invalid ids return a default-constructed WeaponDef (name empty,
        // all-zero settings) rather than invoking undefined behaviour.
        [[nodiscard]] const WeaponDef& def(WeaponId id) const;

    private:
        static constexpr size_t kCount = static_cast<size_t>(WeaponId::Count);
        std::array<WeaponDef, kCount> m_defs;
        WeaponDef                     m_invalid;
    };

    // Process-wide registry instance.
    const WeaponRegistry& registry();
}
