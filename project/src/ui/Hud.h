#pragma once

class Hud
{
public:
    void triggerHitmarker();

    // Update the death/respawn state displayed in the HUD.
    // Call from the snapshot handler when the local player's isAlive changes.
    void setRespawn(bool is_dead, float seconds_remaining);

    // Must be called between ImGuiLayer::beginFrame() and endFrame().
    void draw(float deltaTime);

    [[nodiscard]] bool hitmarkerActive() const { return m_hitmarkerTimer > 0.0f; }

private:
    float m_hitmarkerTimer     = 0.0f;
    bool  m_isDead             = false;
    float m_respawnRemaining   = 0.0f;

    static constexpr float kHitmarkerDuration = 0.18f;
};
