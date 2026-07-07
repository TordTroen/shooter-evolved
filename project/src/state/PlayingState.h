#pragma once

#include "core/GameState.h"
#include "net/InputFrame.h"
#include "net/NetworkId.h"
#include "net/PlayerState.h"
#include "player/CharacterController.h"
#include "player/Weapon.h"
#include "ui/Hud.h"
#include "state/PlayerStats.h"

#include <array>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

union SDL_Event;
class Camera;
class DecalRenderer;
class DemoScene;
class MeshRenderer;
class MuzzleFlashEffect;
class RemotePlayerRenderer;
class ViewmodelRenderer;

class PlayingState : public GameState
{
public:
    explicit PlayingState(Game& game);
    ~PlayingState() override;

    void enter() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt, const bool* keys) override;
    void render() override;
    void renderUI() override;
    std::string name() const override { return "PlayingState"; }

private:
    // Per-tick entry stored in the input ring buffer (plan D3 / NetworkingGuidelines §6).
    struct PredictedFrame
    {
        uint32_t                 tick  = 0;
        InputFrame               input;
        CharacterController::State state;
    };

    // 128 ticks at 60 Hz ≈ 2.1 s — covers any realistic RTT.
    static constexpr int kInputBufferSize = 128;

    std::unique_ptr<DemoScene>           m_scene;
    std::unique_ptr<CharacterController> m_character;
    std::unique_ptr<Camera>              m_camera;
    std::unique_ptr<MeshRenderer>         m_gunMR;
    std::unique_ptr<ViewmodelRenderer>    m_gunViewmodel;
    std::unique_ptr<MuzzleFlashEffect>    m_muzzleFlash;
    std::unique_ptr<DecalRenderer>        m_decals;
    std::unique_ptr<MeshRenderer>         m_remoteBodyMR;
    std::unique_ptr<RemotePlayerRenderer> m_remotePlayerRenderer;
    Weapon                               m_weapon;
    Hud                                  m_hud;

    struct RemotePlayer
    {
        PlayerState                        state;
        std::unique_ptr<MuzzleFlashEffect> muzzleFlash;
    };

    // Remote-player ghost actors (net mode only): NetworkId → latest state + flash.
    std::unordered_map<NetworkId, RemotePlayer> m_remotePlayers;

    // Rebuilt from every snapshot (all players, local included) — see onSnapshot.
    std::unordered_map<NetworkId, PlayerStats> m_scoreStats;
    bool m_showScoreboard = false;

    // Input history ring buffer: stores one entry per predicted tick (plan D3).
    std::array<PredictedFrame, kInputBufferSize> m_inputBuffer{};

    float    m_lastDt              = 0.0f;
    float    m_tickAccum           = 0.0f; // fixed-step accumulator (plan D2)
    uint32_t m_clientTick          = 0;
    bool     m_skipFirstMouseEvent = true;
    bool     m_shouldFire          = false;
    bool     m_isDead              = false;
    float    m_respawnRemaining    = 0.0f;

    // Rewind to the server-authoritative position at acked_tick and replay buffered
    // inputs forward to reproduce the current predicted state (plan D3).
    void reconcile(uint32_t acked_tick, glm::vec3 auth_pos);
};
