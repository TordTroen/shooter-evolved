#pragma once

#include "../weapons/WeaponDef.h"

#include <string>

class Hud
{
public:
    void triggerHitmarker();

    // Update the death/respawn state displayed in the HUD.
    // Call from the snapshot handler when the local player's isAlive changes.
    void setRespawn(bool is_dead, float seconds_remaining);

    // Update the ammo/reload readout from the replicated PlayerState fields.
    // magazineCapacity/reserveAmmoMax are derived from the registry via equipped_weapon,
    // not sent on the wire (WeaponDefinitionsAndFiring.md).
    void setAmmo(weapons::WeaponId equipped_weapon, int ammo_in_mag, int reserve_ammo,
                 float reload_remaining);

    // Shows "pick up weapon <name>" when a replicated weapon item is in pickup range of
    // the local player (MultipleWeapons.md). Call every frame with the current state -
    // visible=false hides the prompt.
    void setPickupPrompt(bool visible, const std::string& weapon_name);

    // Must be called between ImGuiLayer::beginFrame() and endFrame().
    void draw(float deltaTime);

    [[nodiscard]] bool hitmarkerActive() const { return m_hitmarkerTimer > 0.0f; }

private:
    float m_hitmarkerTimer     = 0.0f;
    bool  m_isDead             = false;
    float m_respawnRemaining   = 0.0f;

    weapons::WeaponId m_equippedWeapon  = weapons::kDefaultWeapon;
    int                m_ammoInMag       = 0;
    int                m_reserveAmmo     = 0;
    float              m_reloadRemaining = 0.0f;

    bool        m_pickupPromptVisible = false;
    std::string m_pickupPromptName;

    static constexpr float kHitmarkerDuration = 0.18f;
};
