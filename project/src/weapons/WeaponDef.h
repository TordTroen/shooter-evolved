#pragma once

#include <cstdint>
#include <string>

namespace weapons
{
    // Stable identifier per weapon - replicated to say which weapon a player holds.
    enum class WeaponId : uint8_t
    {
        BasicGun,
        BasicPistol,
        BasicShotgun,

        Count // sentinel, not a real weapon
    };

    constexpr WeaponId kDefaultWeapon = WeaponId::BasicShotgun;

    enum class FireMode : uint8_t
    {
        Semi,
        Auto
    };

    // Immutable description of one weapon: name, model, viewmodel transform, and the
    // firing settings that drive server-authoritative ammo/fire-rate/reload/spread.
    struct WeaponDef
    {
        std::string name;
        std::string modelPath;

        // Viewmodel transform (see ViewmodelRenderer) - per-weapon because each model's
        // origin/scale/forward axis differs.
        float rightOffset    = 0.0f;
        float downOffset     = 0.0f;
        float forwardOffset  = 0.0f;
        float scale          = 1.0f;
        float axisFixDegrees = 0.0f;

        int      magazineCapacity = 1;
        int      reserveAmmoMax   = 0;
        float    reloadSeconds    = 1.0f;
        int      damage           = 0;
        float    impulse          = 0.0f;
        float    fireRate         = 1.0f; // rounds/second
        FireMode fireMode         = FireMode::Semi;
        int      pelletsPerShot   = 1;
        float    spreadDegrees    = 0.0f;
    };
}
