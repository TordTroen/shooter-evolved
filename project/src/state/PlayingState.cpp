#include "PlayingState.h"
#include "core/Game.h"
#include "actor/Actor.h"
#include "actor/components/MeshRenderer.h"
#include "net/InputFrame.h"
#include "net/FireIntent.h"
#include "net/NetRole.h"
#include "rendering/Mesh.h"
#include "net/Net.h"
#include "net/Client.h"
#include "player/CharacterController.h"
#include "rendering/MuzzleFlashEffect.h"
#include "rendering/Shader.h"
#include "rendering/ViewmodelRenderer.h"
#include "scene/Camera.h"
#include "scene/DemoScene.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

PlayingState::PlayingState(Game& game)
    : GameState(game)
{
    m_scene = std::make_unique<DemoScene>(&game.planeMesh(), &game.boxMesh());
    m_scene->setup();

    // In multiplayer, replicated actors are driven by server snapshots — don't
    // simulate them locally so they don't fight the authoritative state.
    const NetRole role = game.net()->role();
    const bool is_multiplayer = (role == NetRole::Host || role == NetRole::Client);
    m_scene->setSimulateReplicated(!is_multiplayer);

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

    // Hook into Client snapshots so remote-player positions and actor states stay current.
    // In solo the snapshot contains only the local player, so m_remotePlayers stays empty.
    m_game.net()->client()->onSnapshot = [this](const SnapshotState& snap) {
        for (auto& [id, ps] : snap.players)
        {
            if (id != m_game.net()->client()->localPlayerId())
                m_remotePlayers[id] = ps;
        }

        // Apply server-authoritative actor states onto local scene actors.
        // Unknown netIds are silently dropped (NetworkingGuidelines §8).
        for (const auto& as : snap.actors)
        {
            Actor* actor = m_scene->findActorByNetId(as.netId);
            if (!actor) { continue; }
            actor->position = as.position;
            actor->rotation = as.rotation;
            actor->syncFromSnapshot(as.health, as.isAlive);
        }
    };
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

    // Gamepad look
    const glm::vec2 look = gamepad.look();
    if (look.x != 0.0f || look.y != 0.0f)
        m_camera->processMouseMotion(look.x, look.y, 1.0f);

    InputFrame input = InputFrame::fromLocal(keys, gamepad, *m_camera, m_clientTick++);
    if (m_shouldFire) { input.buttons |= InputFrame::kButtonFire; }

    Client& client = *m_game.net()->client();

    // Always send input to the server (solo: in-process loopback).
    client.sendInput(input);

    if (m_shouldFire)
    {
        m_shouldFire = false;

        const glm::vec3 eyePos = m_character->position()
            + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f);

        m_muzzleFlash->trigger(); // CHEAT: cosmetic prediction before server confirmation

        // CHEAT: client-side read-only query drives the cosmetic hitmarker only.
        // Props are now server-authoritative and replicated via ActorState in snapshots.
        // No damage or impulse is applied here — the server handles that authoritatively.
        const FireResult predicted = m_weapon.query(*m_scene, eyePos, m_camera->front());
        if (predicted.damaged) { m_hud.triggerHitmarker(); }

        FireIntent intent;
        intent.shooterId  = client.localPlayerId();
        intent.clientTick = input.tick;
        intent.origin     = eyePos; // CHEAT: client-supplied (already marked in FireIntent.h)
        intent.direction  = m_camera->front();
        client.sendFireIntent(intent);
    }

    // CHEAT: local movement prediction; server is authoritative (Level 1 — may drift).
    m_character->simulate(dt, input);

    m_scene->tick(dt);
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

    // Render remote players as simple boxes.
    shader.setInt("uHasBaseColor", 0);
    for (const auto& [id, ps] : m_remotePlayers)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), ps.position);
        model = glm::scale(model, glm::vec3(0.6f, 1.8f, 0.6f));
        shader.setMat4("model", model);
        shader.setVec3("uColor", glm::vec3(1.0f, 0.4f, 0.4f));
        m_game.boxMesh().draw();
    }

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
