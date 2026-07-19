#include "Hud.h"

#include "../weapons/WeaponRegistry.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    constexpr ImGuiWindowFlags kFpsFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs;
    constexpr float kOverlayPad = 2.0f;

    constexpr float kCrosshairSize      = 8.0f;
    constexpr float kCrosshairGap       = 3.0f;
    constexpr float kCrosshairThickness = 2.0f;
    constexpr ImU32 kCrosshairColor     = IM_COL32(255, 255, 255, 220);

    constexpr float kHitmarkerInner     = 5.0f;
    constexpr float kHitmarkerOuter     = 10.0f;
    constexpr float kHitmarkerThickness = 2.0f;
}

void Hud::triggerHitmarker()
{
    m_hitmarkerTimer = kHitmarkerDuration;
}

void Hud::setRespawn(bool is_dead, float seconds_remaining)
{
    m_isDead           = is_dead;
    m_respawnRemaining = seconds_remaining;
}

void Hud::setAmmo(weapons::WeaponId equipped_weapon, int ammo_in_mag, int reserve_ammo,
                   float reload_remaining)
{
    m_equippedWeapon  = equipped_weapon;
    m_ammoInMag       = ammo_in_mag;
    m_reserveAmmo     = reserve_ammo;
    m_reloadRemaining = reload_remaining;
}

void Hud::draw(float deltaTime)
{
    // FPS overlay
    ImGui::SetNextWindowPos({ kOverlayPad, kOverlayPad }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    if (ImGui::Begin("Perf", nullptr, kFpsFlags))
    {
        const ImGuiIO& io = ImGui::GetIO();
        const float frametime = 1000.0f / io.Framerate;
        ImGui::Text("%.1f (%.2f ms)", io.Framerate, frametime);
    }
    ImGui::End();

    // Crosshair
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center{
        viewport->Pos.x + viewport->Size.x * 0.5f,
        viewport->Pos.y + viewport->Size.y * 0.5f,
    };
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->AddLine({ center.x - kCrosshairSize, center.y }, { center.x - kCrosshairGap,  center.y }, kCrosshairColor, kCrosshairThickness);
    drawList->AddLine({ center.x + kCrosshairGap,  center.y }, { center.x + kCrosshairSize, center.y }, kCrosshairColor, kCrosshairThickness);
    drawList->AddLine({ center.x, center.y - kCrosshairSize }, { center.x, center.y - kCrosshairGap  }, kCrosshairColor, kCrosshairThickness);
    drawList->AddLine({ center.x, center.y + kCrosshairGap  }, { center.x, center.y + kCrosshairSize }, kCrosshairColor, kCrosshairThickness);

    // Hitmarker
    if (m_hitmarkerTimer > 0.0f)
    {
        const float alpha = std::clamp(m_hitmarkerTimer / kHitmarkerDuration, 0.0f, 1.0f);
        const ImU32 color = IM_COL32(255, 255, 255, static_cast<int>(230.0f * alpha));
        drawList->AddLine({ center.x - kHitmarkerOuter, center.y - kHitmarkerOuter }, { center.x - kHitmarkerInner, center.y - kHitmarkerInner }, color, kHitmarkerThickness);
        drawList->AddLine({ center.x + kHitmarkerInner, center.y - kHitmarkerInner }, { center.x + kHitmarkerOuter, center.y - kHitmarkerOuter }, color, kHitmarkerThickness);
        drawList->AddLine({ center.x - kHitmarkerOuter, center.y + kHitmarkerOuter }, { center.x - kHitmarkerInner, center.y + kHitmarkerInner }, color, kHitmarkerThickness);
        drawList->AddLine({ center.x + kHitmarkerInner, center.y + kHitmarkerInner }, { center.x + kHitmarkerOuter, center.y + kHitmarkerOuter }, color, kHitmarkerThickness);
        m_hitmarkerTimer -= deltaTime;
    }

    // Ammo text
    ImGui::SetNextWindowPos(
        { viewport->Pos.x + viewport->Size.x - kOverlayPad,
          viewport->Pos.y + viewport->Size.y - kOverlayPad },
        ImGuiCond_Always, { 1.0f, 1.0f });
    ImGui::SetNextWindowBgAlpha(0.0f);
    if (ImGui::Begin("Ammo", nullptr, kFpsFlags))
    {
        if (m_reloadRemaining > 0.0f)
        {
            ImGui::Text("Reloading...");
        }
        ImGui::Text("%d/%d", m_ammoInMag, m_reserveAmmo);
    }
    ImGui::End();

    // Respawn countdown: centered text while the local player is dead.
    // respawnRemaining=0 with isDead=true means no spawn point is available yet.
    if (m_isDead)
    {
        ImGui::SetNextWindowPos(
            { viewport->Pos.x + viewport->Size.x * 0.5f,
              viewport->Pos.y + viewport->Size.y * 0.5f },
            ImGuiCond_Always, { 0.5f, 0.5f });
        ImGui::SetNextWindowBgAlpha(0.0f);
        if (ImGui::Begin("Respawn", nullptr, kFpsFlags))
        {
            if (m_respawnRemaining > 0.0f)
            {
                const int seconds = static_cast<int>(std::ceil(m_respawnRemaining));
                ImGui::Text("Respawning in %d", seconds);
            }
            else
            {
                ImGui::Text("Respawning...");
            }
        }
        ImGui::End();
    }
}
