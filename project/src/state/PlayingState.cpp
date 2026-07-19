#include "PlayingState.h"
#include "core/Game.h"
#include "actor/Actor.h"
#include "actor/components/MeshRenderer.h"
#include "net/FireIntent.h"
#include "net/NetRole.h"
#include "rendering/Mesh.h"
#include "net/Net.h"
#include "net/Client.h"
#include "rendering/DecalRenderer.h"
#include "rendering/MuzzleFlashEffect.h"
#include "rendering/Shader.h"
#include "rendering/RemotePlayerRenderer.h"
#include "rendering/ViewmodelRenderer.h"
#include "scene/Camera.h"
#include "scene/DemoScene.h"
#include "scene/Orientation.h"
#include "ui/Scoreboard.h"
#include "weapons/Pellets.h"
#include "weapons/WeaponRegistry.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <string>
#include <vector>

// Fixed simulation timestep - must match Server::kTickRate (60 Hz).
// Keeping the client and server at the same timestep is a precondition for
// deterministic prediction and reconciliation (NetworkingGuidelines §5 / plan D2).
static constexpr float kSimTickRate = 60.0f;
static constexpr float kSimTickDt   = 1.0f / kSimTickRate;

// Positional error below this threshold is ignored during reconciliation to avoid
// correcting negligible floating-point drift from the moving-prop contact residual (plan D3).
static constexpr float kReconcileThreshold = 0.05f; // 5 cm

PlayingState::PlayingState(Game& game)
    : GameState(game)
{
    m_scene = std::make_unique<DemoScene>(&game.planeMesh(), &game.boxMesh(),
        [](const Mesh* mesh, glm::vec3 color) {
            return std::make_unique<MeshRenderer>(mesh, color);
        });
    m_scene->setup();

    // In multiplayer, replicated actors are driven by server snapshots - don't
    // simulate them locally so they don't fight the authoritative state.
    const NetRole role = game.net()->role();
    const bool is_multiplayer = (role == NetRole::Host || role == NetRole::Client);
    m_scene->setSimulateReplicated(!is_multiplayer);

    // Phase 1 (D1): convert replicated actor bodies to kinematic collision proxies on
    // the client. They are driven to the snapshot transform each tick via moveKinematic
    // in Scene::tick, so the player's CharacterVirtual collides against correct shapes.
    // The server retains the true motion types (dynamic/kinematic) as sole integrator (§1).
    if (is_multiplayer)
    {
        m_scene->make_replicated_bodies_kinematic();
    }

    m_character = std::make_unique<CharacterController>(
        m_scene->physics(), glm::vec3(0.0f, 2.0f, 8.0f));
    m_camera = std::make_unique<Camera>(
        glm::vec3(0.0f, 2.0f + CharacterController::eyeHeight(), 8.0f));

    m_remoteBodyMR = std::make_unique<MeshRenderer>(&game.boxMesh(), glm::vec3(1.0f, 0.4f, 0.4f));

    const weapons::WeaponDef& equippedDef = weapons::registry().def(weapons::kDefaultWeapon);
    m_weapon = Weapon(equippedDef);

    auto& gunModel = game.gunModel();
    if (gunModel.mesh)
    {
        m_gunMR          = std::make_unique<MeshRenderer>(gunModel.mesh.get(), glm::vec3(1.0f));
        m_gunMR->texture = gunModel.baseColorTexture.get();
        m_gunViewmodel   = std::make_unique<ViewmodelRenderer>(*m_gunMR,
            equippedDef.rightOffset, equippedDef.downOffset, equippedDef.forwardOffset,
            equippedDef.scale, equippedDef.axisFixDegrees);
        m_remotePlayerRenderer = std::make_unique<RemotePlayerRenderer>(*m_remoteBodyMR, *m_gunMR);
    }
    m_muzzleFlash = std::make_unique<MuzzleFlashEffect>(game.planeMesh(), game.muzzleFlashTexture());
    m_decals      = std::make_unique<DecalRenderer>(game.planeMesh(), game.decalTexture());

    // Hook into Client snapshots so remote-player positions and actor states stay current.
    // In solo the snapshot contains only the local player, so m_remotePlayers stays empty.
    m_game.net()->client()->onSnapshot = [this](const SnapshotState& snap) {
        const NetworkId local_id = m_game.net()->client()->localPlayerId();

        // Scoreboard stats: rebuilt from every player in the snapshot (local included -
        // this is the only place the local player's kills/deaths are captured, since the
        // rest of onSnapshot only extracts remote ghosts / isAlive-for-HUD below).
        m_scoreStats.clear();
        for (auto& [id, ps] : snap.players)
        {
            m_scoreStats[id] = { ps.kills, ps.deaths };
        }

        for (auto& [id, ps] : snap.players)
        {
            if (id != local_id)
            {
                auto it = m_remotePlayers.find(id);
                if (it == m_remotePlayers.end())
                {
                    RemotePlayer rp;
                    rp.state = ps;
                    rp.muzzleFlash = std::make_unique<MuzzleFlashEffect>(
                        m_game.planeMesh(), m_game.muzzleFlashTexture());
                    m_remotePlayers.emplace(id, std::move(rp));
                }
                else
                {
                    if (ps.fireCount != it->second.state.fireCount)
                    {
                        it->second.muzzleFlash->trigger();

                        // CHEAT: cosmetic client-side ray from the remote player's
                        // last-known position/aim to place a bullet-hole decal.
                        const glm::vec3 eye_pos = ps.position
                            + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f);
                        const glm::vec3 front = orientation::basis_from_yaw_pitch(
                            ps.yaw, ps.pitch).front;
                        const FireResult remote_shot = m_weapon.query(*m_scene, eye_pos, front);
                        if (remote_shot.hit && !remote_shot.hitActor)
                        {
                            m_decals->add(remote_shot.position, remote_shot.normal);
                        }
                    }
                    it->second.state = ps;
                }
            }
            else
            {
                // Phase 3 (D3): reconcile local player toward the server-authoritative
                // position at lastProcessedInputTick. CHEAT: client displays the replayed
                // predicted position until the server confirms it.
                reconcile(ps.lastProcessedInputTick, ps.position);

                // Track death/respawn state for movement lock and HUD countdown.
                m_isDead           = !ps.isAlive;
                m_respawnRemaining = ps.respawnRemaining;
                m_hud.setRespawn(m_isDead, m_respawnRemaining);
                m_hud.setAmmo(ps.equippedWeapon, ps.ammoInMag, ps.reserveAmmo, ps.reloadRemaining);
                m_ammoInMag       = ps.ammoInMag;
                m_reloadRemaining = ps.reloadRemaining;
            }
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
            if (event.button.button == SDL_BUTTON_LEFT) m_mouseHeld = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) m_mouseHeld = false;
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

    // Hold-to-show scoreboard: purely client-side UI toggle, never touches InputFrame
    // or the wire (NetworkingGuidelines - client UI state stays off the network).
    m_showScoreboard = keys[SDL_SCANCODE_TAB];

    auto& gamepad = m_game.gamepad();

    // Gamepad look
    const glm::vec2 look = gamepad.look();
    if (look.x != 0.0f || look.y != 0.0f)
        m_camera->processMouseMotion(look.x, look.y, 1.0f);

    Client& client = *m_game.net()->client();

    const weapons::WeaponDef& weaponDef = weapons::registry().def(weapons::kDefaultWeapon);

    // Phase 2 (D2): fixed-step prediction - sample input once per render frame, then
    // step CharacterController at kSimTickDt (= server's kTickRate). This removes the
    // frame-rate-dependent divergence that made contact with dynamic objects non-deterministic.
    // CHEAT: client renders unconfirmed predicted position until the server acks it (D3).

    // Trigger held this frame - suppressed while dead, reloading, or out of ammo so no
    // FireIntent is sent and no cosmetic flash triggers (CHEAT: mirrors the server's
    // can_fire() gate using the last-known replicated ammo/reload state). Level state
    // (not edge) so should_fire() can branch on Semi (one shot per press) vs Auto
    // (repeated shots while held).
    const bool held_this_frame = !m_isDead && m_ammoInMag > 0 && m_reloadRemaining <= 0.0f
        && (m_mouseHeld || gamepad.fireHeld());

    m_tickAccum += dt;
    while (m_tickAccum >= kSimTickDt)
    {
        m_tickAccum -= kSimTickDt;

        InputFrame input = InputFrame::fromLocal(keys, gamepad, *m_camera, m_clientTick++);

        // When dead, zero movement and suppress jump so the client prediction matches the
        // server's frozen position. Yaw/pitch are kept so free-look still works.
        if (m_isDead)
        {
            input.move    = glm::vec2(0.0f, 0.0f);
            input.buttons &= static_cast<uint8_t>(~InputFrame::kButtonJump);
        }

        // Client self-limits to the weapon's fire mode/rate so full-auto *feels* right;
        // the server's can_fire() clamp is authoritative (WeaponDefinitionsAndFiring.md).
        const bool fire_now = weapons::should_fire(m_fireEdge, held_this_frame, input.tick, weaponDef);
        if (fire_now)
        {
            input.buttons |= InputFrame::kButtonFire;
        }

        // Always send input to the server (solo: in-process loopback).
        client.sendInput(input);

        m_character->simulate(kSimTickDt, input);

        // Phase 3 (D3): store (tick, input, post-sim state) in the ring buffer so
        // reconciliation can replay from the server-acked tick forward.
        m_inputBuffer[input.tick % kInputBufferSize] = { input.tick, input, m_character->state() };

        if (fire_now)
        {
            fireWeapon(client, weaponDef, input.tick);
        }
    }

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

    for (auto& [id, rp] : m_remotePlayers)
    {
        if (!rp.state.isAlive) { continue; } // hide dead remote bodies

        if (m_remotePlayerRenderer)
        {
            m_remotePlayerRenderer->render(shader, rp.state);
            if (rp.muzzleFlash)
            {
                rp.muzzleFlash->render(shader,
                    m_remotePlayerRenderer->muzzle_world_pos(rp.state),
                    m_camera->right(), m_camera->up(), -m_camera->front(), m_lastDt);
            }
        }
        else
        {
            // Fallback when the gun model failed to load: plain body box.
            // rp.state.position is feet-level; offset up by half the body height so the
            // bottom of the unit-cube mesh aligns with the feet.
            const glm::vec3 body_pos = rp.state.position + glm::vec3(0.0f, 0.9f, 0.0f);
            const glm::mat4 body_model =
                glm::translate(glm::mat4(1.0f), body_pos) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rp.state.yaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 1.8f, 0.6f));
            m_remoteBodyMR->draw(shader, body_model);
        }
    }

    m_decals->render(shader);

    if (m_gunViewmodel && !m_isDead)
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

    Client& client = *m_game.net()->client();
    const NetworkId local_id = client.localPlayerId();
    const std::vector<RosterEntry>& roster = client.roster();

    std::vector<ScoreboardEntry> entries;
    entries.reserve(m_scoreStats.size());
    for (const auto& [id, stats] : m_scoreStats)
    {
        const auto it = std::find_if(roster.begin(), roster.end(),
            [id](const RosterEntry& r) { return r.netId == id; });
        const std::string name = (it != roster.end()) ? it->name : ("Player " + std::to_string(id.value));
        const bool is_local = (id == local_id);
        ScoreboardEntry entry(id, name, stats, is_local);
        entries.push_back(std::move(entry));
    }

    const std::vector<ScoreboardEntry> sorted = sort_by_kills(std::move(entries));

    // Always-visible top-center summary: your score + the leading player's score.
    draw_score_summary(sorted, local_id);

    // Full scoreboard only while Tab is held.
    if (m_showScoreboard)
    {
        draw_scoreboard(sorted);
    }
}

void PlayingState::fireWeapon(Client& client, const weapons::WeaponDef& def, uint32_t fire_tick)
{
    const glm::vec3 eyePos = m_character->position()
        + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f);

    m_muzzleFlash->trigger(); // CHEAT: cosmetic prediction before server confirmation

    // CHEAT: client-side read-only query drives the cosmetic hitmarker/decals only.
    // Props are now server-authoritative and replicated via ActorState in snapshots.
    // No damage or impulse is applied here - the server handles that authoritatively.
    // Pellets use the client's own RNG - divergence from the server's authoritative
    // pattern is fine since this is purely visual (WeaponDefinitionsAndFiring.md #4).
    const std::vector<glm::vec3> pellets =
        weapons::pellet_directions(m_camera->front(), def, m_pelletRng);
    for (const glm::vec3& pellet_dir : pellets)
    {
        const FireResult predicted = m_weapon.query(*m_scene, eyePos, pellet_dir);
        if (predicted.damaged) { m_hud.triggerHitmarker(); }
        if (predicted.hit && !predicted.hitActor)
        {
            m_decals->add(predicted.position, predicted.normal);
        }
    }

    FireIntent intent;
    intent.shooterId  = client.localPlayerId();
    intent.clientTick = fire_tick;
    intent.origin     = eyePos; // CHEAT: client-supplied (already marked in FireIntent.h)
    intent.direction  = m_camera->front();
    client.sendFireIntent(intent);
}

void PlayingState::reconcile(uint32_t acked_tick, glm::vec3 auth_pos)
{
    // Guard: if the server somehow echoes a tick we haven't predicted yet, skip.
    if (acked_tick >= m_clientTick) { return; }

    // Check predicted position at the acked tick against the authoritative position.
    // If the error is below threshold, skip reconciliation to avoid correcting negligible
    // drift from moving-prop contact residual (plan D3).
    const PredictedFrame& acked_frame = m_inputBuffer[acked_tick % kInputBufferSize];
    if (acked_frame.tick == acked_tick)
    {
        const float error = glm::length(acked_frame.state.position - auth_pos);
        if (error < kReconcileThreshold) { return; }
    }

    // Rewind to the server-authoritative state at acked_tick.
    // V1: server sends only position; zero velocity forces a clean replay starting from
    // the auth position. The replayed physics will quickly re-acquire ground velocity.
    CharacterController::State auth_state;
    auth_state.position = auth_pos;
    auth_state.velocity = glm::vec3(0.0f);
    m_character->set_state(auth_state);

    // Replay buffered inputs strictly after acked_tick (§6: off-by-one must not replay
    // the acked tick itself - it is already included in the server's authoritative state).
    for (uint32_t t = acked_tick + 1; t < m_clientTick; ++t)
    {
        const PredictedFrame& frame = m_inputBuffer[t % kInputBufferSize];
        if (frame.tick == t) // valid entry (not overwritten by a later tick)
        {
            m_character->simulate(kSimTickDt, frame.input);
        }
    }
}
