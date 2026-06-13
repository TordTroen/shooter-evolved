#pragma once

#include "core/GameState.h"
#include "net/NetworkId.h"
#include "net/PlayerState.h"
#include "player/Weapon.h"
#include "ui/Hud.h"

#include <memory>
#include <unordered_map>

union SDL_Event;
class Camera;
class CharacterController;
class DemoScene;
class MeshRenderer;
class MuzzleFlashEffect;
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

private:
    std::unique_ptr<DemoScene>           m_scene;
    std::unique_ptr<CharacterController> m_character;
    std::unique_ptr<Camera>              m_camera;
    std::unique_ptr<MeshRenderer>        m_gunMR;
    std::unique_ptr<ViewmodelRenderer>   m_gunViewmodel;
    std::unique_ptr<MuzzleFlashEffect>   m_muzzleFlash;
    Weapon                               m_weapon;
    Hud                                  m_hud;

    // Remote-player ghost actors (net mode only): NetworkId → latest state.
    std::unordered_map<NetworkId, PlayerState> m_remotePlayers;

    float    m_lastDt              = 0.0f;
    uint32_t m_clientTick          = 0;
    bool     m_skipFirstMouseEvent = true;
    bool     m_shouldFire          = false;
};
