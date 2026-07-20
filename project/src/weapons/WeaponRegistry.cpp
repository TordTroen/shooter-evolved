#include "WeaponRegistry.h"

namespace weapons
{
    WeaponRegistry::WeaponRegistry()
    {
        {
            WeaponDef& d      = m_defs[static_cast<size_t>(WeaponId::BasicGun)];
            d.name            = "Basic Gun";
            d.modelPath       = "assets/models/BasicGun.glb";
            d.rightOffset     = 0.25f;
            d.downOffset      = -0.20f;
            d.forwardOffset   = 0.45f;
            d.scale           = 0.25f;
            d.axisFixDegrees  = -90.0f;
            d.magazineCapacity = 30;
            d.reserveAmmoMax  = 90;
            d.reloadSeconds   = 2.0f;
            d.damage          = 10;
            d.impulse         = 50.0f;
            d.fireRate        = 8.0f;
            d.fireMode        = FireMode::Auto;
            d.pelletsPerShot  = 1;
            d.spreadDegrees   = 0.0f;
            d.dropBoxHalfExtents = glm::vec3(0.45f, 0.09f, 0.02f);
            d.dropSpinImpulse    = 0.15f;
        }
        {
            WeaponDef& d      = m_defs[static_cast<size_t>(WeaponId::BasicPistol)];
            d.name            = "Basic Pistol";
            d.modelPath       = "assets/models/BasicPistol.glb";
            d.rightOffset     = 0.25f;
            d.downOffset      = -0.20f;
            d.forwardOffset   = 0.45f;
            d.scale           = 0.25f;
            d.axisFixDegrees  = -90.0f;
            d.magazineCapacity = 12;
            d.reserveAmmoMax  = 48;
            d.reloadSeconds   = 1.5f;
            d.damage          = 25;
            d.impulse         = 50.0f;
            d.fireRate        = 4.0f;
            d.fireMode        = FireMode::Semi;
            d.pelletsPerShot  = 1;
            d.spreadDegrees   = 0.0f;
            d.dropBoxHalfExtents = glm::vec3(0.20f, 0.07f, 0.02f);
            d.dropSpinImpulse    = 0.1f;
        }
        {
            WeaponDef& d      = m_defs[static_cast<size_t>(WeaponId::BasicShotgun)];
            d.name            = "Basic Shotgun";
            d.modelPath       = "assets/models/BasicShotgun.glb";
            d.rightOffset     = 0.25f;
            d.downOffset      = -0.20f;
            d.forwardOffset   = 0.45f;
            d.scale           = 0.25f;
            d.axisFixDegrees  = -90.0f;
            d.magazineCapacity = 6;
            d.reserveAmmoMax  = 24;
            d.reloadSeconds   = 2.5f;
            d.damage          = 11;
            d.impulse         = 40.0f;
            d.fireRate        = 1.2f;
            d.fireMode        = FireMode::Semi;
            d.pelletsPerShot  = 8;
            d.spreadDegrees   = 5.0f;
            d.dropBoxHalfExtents = glm::vec3(0.45f, 0.1f, 0.02f);
            d.dropSpinImpulse    = 0.2f;
        }
    }

    const WeaponDef& WeaponRegistry::def(WeaponId id) const
    {
        const size_t idx = static_cast<size_t>(id);
        if (idx >= kCount)
        {
            return m_invalid;
        }
        return m_defs[idx];
    }

    const WeaponRegistry& registry()
    {
        static const WeaponRegistry instance;
        return instance;
    }
}
