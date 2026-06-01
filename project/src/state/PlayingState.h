#pragma once

#include "core/GameState.h"
#include "player/Weapon.h"
#include "ui/Hud.h"

#include <memory>

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

    float m_lastDt               = 0.0f;
    bool  m_skipFirstMouseEvent  = true;
    bool  m_shouldFire           = false;
};
