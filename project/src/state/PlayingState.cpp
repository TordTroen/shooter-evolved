#include "PlayingState.h"
#include "core/Game.h"
#include "actor/Actor.h"
#include "actor/components/MeshRenderer.h"
#include "player/CharacterController.h"
#include "rendering/MuzzleFlashEffect.h"
#include "rendering/Shader.h"
#include "rendering/ViewmodelRenderer.h"
#include "scene/Camera.h"
#include "scene/DemoScene.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

PlayingState::PlayingState(Game& game)
    : GameState(game)
{
    m_scene = std::make_unique<DemoScene>(game.planeMesh(), game.boxMesh());
    m_scene->setup();

    m_character = std::make_unique<CharacterController>(
        m_scene->physics(), glm::vec3(0.0f, 2.0f, 8.0f));
    m_camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 2.0f + CharacterController::eyeHeight(), 8.0f));

    auto& gunModel = game.gunModel();
    if (gunModel.mesh)
    {
        m_gunMR          = std::make_unique<MeshRenderer>(gunModel.mesh.get(), glm::vec3(1.0f));
        m_gunMR->texture = gunModel.baseColorTexture.get();
        m_gunViewmodel   = std::make_unique<ViewmodelRenderer>(*m_gunMR);
    }
    m_muzzleFlash = std::make_unique<MuzzleFlashEffect>(game.planeMesh(), game.muzzleFlashTexture());
}

PlayingState::~PlayingState() = default;

void PlayingState::enter()
{
    SDL_SetWindowRelativeMouseMode(m_game.window().handle(), true);
    m_skipFirstMouseEvent = true;
}

void PlayingState::handleEvent(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT) m_shouldFire = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (m_skipFirstMouseEvent) { m_skipFirstMouseEvent = false; break; }
            m_camera->processMouseMotion(
                static_cast<float>(event.motion.xrel),
                static_cast<float>(event.motion.yrel));
            break;
        default: break;
    }
}

void PlayingState::update(float dt, const bool* keys)
{
    m_lastDt = dt;

    auto& gamepad = m_game.gamepad();
    if (gamepad.fire()) m_shouldFire = true;
    const glm::vec2 look = gamepad.look();
    if (look.x != 0.0f || look.y != 0.0f)
        m_camera->processMouseMotion(look.x, look.y, 1.0f);

    if (m_shouldFire)
    {
        m_shouldFire = false;
        m_muzzleFlash->trigger();
        const FireResult result = m_weapon.fire(*m_scene,
            m_character->position() + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f),
            m_camera->front());
        if (result.damaged) m_hud.triggerHitmarker();
    }

    m_scene->tick(dt);
    m_character->update(dt, keys, m_camera->front(), gamepad.jump(), gamepad.move());
    m_camera->setPosition(m_character->position()
        + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f));

    m_game.shader().checkHotReload();
}

void PlayingState::render()
{
    auto& shader = m_game.shader();
    shader.use();
    shader.setMat4("view",       m_camera->getViewMatrix());
    shader.setMat4("projection", m_game.projection());

    for (const auto& actor : m_scene->actors())
        if (actor->meshRenderer)
            actor->meshRenderer->draw(shader, actor->modelMatrix());

    if (m_gunViewmodel)
    {
        m_gunViewmodel->render(shader, *m_camera);
        m_muzzleFlash->render(shader,
            m_gunViewmodel->muzzleWorldPos(*m_camera, 0.65f),
            m_camera->right(), m_camera->up(), -m_camera->front(), m_lastDt);
    }
}

void PlayingState::renderUI()
{
    m_hud.draw(m_lastDt);
}
