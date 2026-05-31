#pragma once

class Hud
{
public:
    void triggerHitmarker();

    // Must be called between ImGuiLayer::beginFrame() and endFrame().
    void draw(float deltaTime);

    [[nodiscard]] bool hitmarkerActive() const { return m_hitmarkerTimer > 0.0f; }

private:
    float m_hitmarkerTimer = 0.0f;
    static constexpr float kHitmarkerDuration = 0.18f;
};
