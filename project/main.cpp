#include "actor/Actor.h"
#include "actor/components/MeshRenderer.h"
#include "actor/components/PhysicsBody.h"
#include "core/Window.h"
#include "physics/PhysicsWorld.h"
#include "player/CharacterController.h"
#include "rendering/GLDebug.h"
#include "rendering/Mesh.h"
#include "rendering/ModelLoader.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "rendering/Vertex.h"
#include "scene/Camera.h"
#include "scene/DemoScene.h"
#include "ui/ImGuiLayer.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace Debug::Settings
{
    constexpr bool kEnableHitPositionMarker = false;
}

static void setProjectRootAsWorkingDir()
{
    namespace fs = std::filesystem;
    const char* exeDir = SDL_GetBasePath();
    if (!exeDir)
    {
        return;
    }

    fs::path dir = exeDir;
    while (dir.has_parent_path())
    {
        if (fs::exists(dir / "shaders"))
        {
            fs::current_path(dir);
            std::cerr << "[Init] Working directory set to: " << dir << "\n";
            return;
        }
        fs::path parent = dir.parent_path();
        if (parent == dir)
        {
            break;
        }
        dir = parent;
    }
    std::cerr << "[Init] WARNING: Could not find project root (no 'shaders/' found walking up from exe).\n";
}

// Unit quad in the XZ plane (y=0), normals pointing up.
static constexpr std::array<Vertex, 4> kPlaneVerts = {{
    { {-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
    { { 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
    { { 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    { {-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
}};
static constexpr std::array<uint32_t, 6> kPlaneIdx = { 0, 1, 2,  0, 2, 3 };

// Unit box centred at origin (-0.5..0.5 on each axis).
static constexpr std::array<Vertex, 24> kBoxVerts = {{
    // +X face
    { { 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },
    { { 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
    { { 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
    { { 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
    // -X face
    { {-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} },
    { {-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} },
    { {-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} },
    { {-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} },
    // +Y face
    { {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f} },
    { { 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f} },
    { { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f} },
    { {-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f} },
    // -Y face
    { {-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f} },
    { { 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f} },
    { { 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f} },
    { {-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f} },
    // +Z face
    { { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} },
    { {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} },
    { {-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f} },
    { { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f} },
    // -Z face
    { {-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f} },
    { { 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f} },
    { { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f} },
    { {-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f} },
}};
// Procedural muzzle flash: radial falloff with a few angular spikes, white-hot
// core fading to orange. Alpha is used for blending; RGB is pre-tinted so
// additive blending lights the scene the right colour without a uniform tweak.
static std::vector<uint8_t> makeMuzzleFlashPixels(int size)
{
    std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);
    const float center = (size - 1) * 0.5f;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float dx = (x - center) / center;
            const float dy = (y - center) / center;
            const float d  = std::sqrt(dx * dx + dy * dy);

            const float angle  = std::atan2(dy, dx);
            const float spikes = 0.85f + 0.15f * std::cos(angle * 5.0f);
            const float radial = std::clamp(1.0f - d / spikes, 0.0f, 1.0f);
            const float core   = std::pow(radial, 2.2f);

            const float r = std::clamp(core * 1.6f, 0.0f, 1.0f);
            const float g = std::clamp(core * 1.1f, 0.0f, 1.0f);
            const float b = std::clamp(core * 0.5f, 0.0f, 1.0f);
            const float a = core;

            const size_t i = (static_cast<size_t>(y) * size + x) * 4;
            pixels[i + 0] = static_cast<uint8_t>(r * 255.0f);
            pixels[i + 1] = static_cast<uint8_t>(g * 255.0f);
            pixels[i + 2] = static_cast<uint8_t>(b * 255.0f);
            pixels[i + 3] = static_cast<uint8_t>(a * 255.0f);
        }
    }
    return pixels;
}

static constexpr std::array<uint32_t, 36> kBoxIdx = {
     0,  1,  2,   0,  2,  3,   // +X
     4,  5,  6,   4,  6,  7,   // -X
     8,  9, 10,   8, 10, 11,   // +Y
    12, 13, 14,  12, 14, 15,   // -Y
    16, 17, 18,  16, 18, 19,   // +Z
    20, 21, 22,  20, 22, 23,   // -Z
};

int main(int /*argc*/, char* /*argv*/[])
{
    std::cout << "[Init] Starting...\n";
    setProjectRootAsWorkingDir();

    Window window({ .title = "FPS Demo", .width = 1280, .height = 720 });
    SDL_SetWindowRelativeMouseMode(window.handle(), true);

    wireUpGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    ImGuiLayer imguiLayer(window);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Mesh   planeMesh(kPlaneVerts, kPlaneIdx);
    Mesh   boxMesh(kBoxVerts, kBoxIdx);

    // 1x1 white texture bound to unit 0 by default. Silences the GL validator
    // warning about non-textured actors leaving sampler `uBaseColor` pointing
    // at a unit with no texture; `uHasBaseColor` still controls actual use.
    constexpr uint8_t kWhitePixel[4] = { 255, 255, 255, 255 };
    Texture defaultWhiteTexture(kWhitePixel, 1, 1, 4);
    defaultWhiteTexture.bind(0);

    // Load a glTF/GLB model — must outlive the renderer that references it.
    ModelLoader::LoadedModel gunModel = ModelLoader::loadGltf("assets/models/BasicGun.glb");

    // Procedurally generated muzzle flash texture (alpha-blended billboard).
    constexpr int          kMuzzleFlashTexSize = 128;
    const std::vector<uint8_t> muzzleFlashPixels = makeMuzzleFlashPixels(kMuzzleFlashTexSize);
    Texture muzzleFlashTexture(muzzleFlashPixels.data(), kMuzzleFlashTexSize, kMuzzleFlashTexSize, 4);

    DemoScene scene(planeMesh, boxMesh);
    scene.setup();

    // Viewmodel: gun is rendered each frame in camera-relative space, not as a
    // scene actor. Its transform is derived from the camera basis below.
    std::unique_ptr<MeshRenderer> gunViewmodel;
    if (gunModel.mesh)
    {
        gunViewmodel = std::make_unique<MeshRenderer>(gunModel.mesh.get(), glm::vec3(1.0f));
        gunViewmodel->texture = gunModel.baseColorTexture.get();
    }

    CharacterController character(scene.physics(), glm::vec3(0.0f, 2.0f, 8.0f));

    const glm::vec3 startFeet(0.0f, 2.0f, 8.0f);
    Camera camera(startFeet + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f));

    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        static_cast<float>(window.width()) / static_cast<float>(window.height()),
        0.1f, 1000.0f);

    bool  skipFirstMouseEvent = true;
    bool  shouldFire          = false;
    float hitmarkerTimer      = 0.0f; // seconds remaining; >0 draws fading hitmarker
    float muzzleFlashTimer    = 0.0f; // seconds remaining; >0 draws muzzle flash quad
    constexpr float kMuzzleFlashDuration = 0.2f;

    Uint64 lastTime = SDL_GetTicksNS();

    while (!window.shouldClose())
    {
        const Uint64 now       = SDL_GetTicksNS();
        const float  deltaTime = std::min(static_cast<float>(now - lastTime) * 1e-9f, 0.05f);
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            imguiLayer.processEvent(event);
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    window.setShouldClose(true);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        window.setShouldClose(true);
                    }
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    window.onResize(event.window.data1, event.window.data2);
                    projection = glm::perspective(
                        glm::radians(60.0f),
                        static_cast<float>(event.window.data1) / static_cast<float>(event.window.data2),
                        0.1f, 1000.0f);
                    glViewport(0, 0, event.window.data1, event.window.data2);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        shouldFire = true;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (skipFirstMouseEvent)
                    {
                        skipFirstMouseEvent = false;
                        break;
                    }
                    camera.processMouseMotion(
                        static_cast<float>(event.motion.xrel),
                        static_cast<float>(event.motion.yrel));
                    break;
                default:
                    break;
            }
        }

        int numKeys = 0;
        const bool* keys = SDL_GetKeyboardState(&numKeys);

        if (shouldFire)
        {
            shouldFire         = false;
            muzzleFlashTimer   = kMuzzleFlashDuration;
            constexpr int   kShotDamage  = 25;
            constexpr float kShotImpulse = 50.0f; // kg·m/s along shot direction
            const glm::vec3 eyePos = character.position() + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f);
            const RayHit hit = scene.physics().castRay(eyePos, camera.front());
            if (hit.hit)
            {
                std::cout << "[Shot] hit at ("
                    << hit.position.x << ", "
                    << hit.position.y << ", "
                    << hit.position.z << ")\n";

                if (Actor* target = scene.findActorByBodyID(hit.bodyID))
                {
                    if (target->isDamageable())
                    {
                        target->takeDamage(kShotDamage);
                        std::cout << "[Damage] " << kShotDamage << " dmg -> "
                            << target->health << "/" << target->maxHealth
                            << (target->isPendingDestroy() ? " (destroyed)" : "")
                            << "\n";
                        constexpr float kHitmarkerDuration = 0.18f;
                        hitmarkerTimer = kHitmarkerDuration;
                    }
                    if (target->physicsBody)
                    {
                        target->physicsBody->applyImpulse(camera.front() * kShotImpulse, hit.position);
                    }
                }

                if (Debug::Settings::kEnableHitPositionMarker)
                {
                    auto marker      = std::make_unique<Actor>();
                    marker->position = hit.position;
                    marker->scale    = glm::vec3(0.1f);
                    marker->meshRenderer = std::make_unique<MeshRenderer>(&boxMesh, glm::vec3(1.0f, 0.2f, 0.2f));
                    scene.spawn(std::move(marker));
                }
            }
        }

        // 1. Advance actor logic + step physics (kinematics reach targets, collisions resolved).
        scene.tick(deltaTime);

        // 2. Resolve character against the now-current body positions.
        character.update(deltaTime, keys, camera.front());

        // Sync camera to character eye position.
        const glm::vec3 feet = character.position();
        camera.setPosition(feet + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f));

        shader.checkHotReload();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("view",       camera.getViewMatrix());
        shader.setMat4("projection", projection);

        for (const auto& actor : scene.actors())
        {
            if (actor->meshRenderer)
                actor->meshRenderer->draw(shader, actor->modelMatrix());
        }

        // Viewmodel: position the gun in the bottom-right of the view, pointing
        // forward along the camera. Depth is cleared first so the gun never
        // clips into nearby world geometry (standard FPS technique).
        if (gunViewmodel)
        {
            constexpr float kRightOffset   =  0.25f;
            constexpr float kDownOffset    = -0.20f;
            constexpr float kForwardOffset =  0.45f;
            constexpr float kGunScale      =  0.25f;

            const glm::vec3 camPos   = camera.position();
            const glm::vec3 camFront = camera.front();
            const glm::vec3 camRight = glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f)));
            const glm::vec3 camUp    = glm::normalize(glm::cross(camRight, camFront));

            const glm::vec3 gunPos = camPos
                + camRight * kRightOffset
                + camUp    * kDownOffset
                + camFront * kForwardOffset;

            // Build rotation from camera basis. glTF convention is +Y up,
            // -Z forward, so the model's -Z must map to camera front.
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(camRight,  0.0f);
            rot[1] = glm::vec4(camUp,     0.0f);
            rot[2] = glm::vec4(-camFront, 0.0f);

            // BasicGun.glb's barrel points along its local -X, not -Z. Yaw the
            // model -90° around Y so its barrel ends up along local -Z, which
            // the camera basis above then maps to camera-forward.
            const glm::mat4 modelOrientationFix =
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            const glm::mat4 gunModelMat =
                glm::translate(glm::mat4(1.0f), gunPos) *
                rot *
                modelOrientationFix *
                glm::scale(glm::mat4(1.0f), glm::vec3(kGunScale));

            glClear(GL_DEPTH_BUFFER_BIT);
            gunViewmodel->draw(shader, gunModelMat);

            if (muzzleFlashTimer > 0.0f)
            {
                // Position the flash at the gun's barrel tip, billboarded toward
                // the camera. kPlaneVerts lies in the XZ plane with +Y normal,
                // so we map plane-Y to -camFront to face the camera.
                constexpr float kFlashForwardOffset = 0.65f; // past the barrel tip
                constexpr float kFlashSize          = 0.30f;

                const float t     = muzzleFlashTimer / kMuzzleFlashDuration;
                const float fade  = std::clamp(t, 0.0f, 1.0f);
                const float scale = kFlashSize * (0.8f + 0.4f * fade); // small pop

                const glm::vec3 flashPos = camPos
                    + camRight * kRightOffset
                    + camUp    * kDownOffset
                    + camFront * kFlashForwardOffset;

                glm::mat4 billboard(1.0f);
                billboard[0] = glm::vec4(camRight,  0.0f);
                billboard[1] = glm::vec4(-camFront, 0.0f); // plane normal -> camera
                billboard[2] = glm::vec4(camUp,     0.0f);

                const glm::mat4 flashMat =
                    glm::translate(glm::mat4(1.0f), flashPos) *
                    billboard *
                    glm::scale(glm::mat4(1.0f), glm::vec3(scale));

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
                glDepthMask(GL_FALSE);

                muzzleFlashTexture.bind(0);
                shader.setInt ("uBaseColor",    0);
                shader.setInt ("uHasBaseColor", 1);
                shader.setInt ("uEmissive",     1);
                shader.setVec3("uColor",        glm::vec3(fade));
                shader.setMat4("model",         flashMat);
                planeMesh.draw();
                shader.setInt ("uEmissive",     0);

                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);

                muzzleFlashTimer -= deltaTime;
            }
        }

        imguiLayer.beginFrame();
        {
            constexpr ImGuiWindowFlags kFpsFlags =
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoInputs;
            constexpr float kOverlayPad = 2.0f;
            ImGui::SetNextWindowPos({ kOverlayPad, kOverlayPad }, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);
            if (ImGui::Begin("Perf", nullptr, kFpsFlags))
            {
                const ImGuiIO& io = ImGui::GetIO();
                const float frametime = 1000.0f / io.Framerate;
                ImGui::Text("%.1f (%.2f ms)", io.Framerate, frametime);
            }
            ImGui::End();

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImVec2 center{
                viewport->Pos.x + viewport->Size.x * 0.5f,
                viewport->Pos.y + viewport->Size.y * 0.5f,
            };
            constexpr float kCrosshairSize = 8.0f;
            constexpr float kCrosshairGap  = 3.0f;
            constexpr float kCrosshairThickness = 2.0f;
            constexpr ImU32 kCrosshairColor = IM_COL32(255, 255, 255, 220);
            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            drawList->AddLine({ center.x - kCrosshairSize, center.y }, { center.x - kCrosshairGap,  center.y }, kCrosshairColor, kCrosshairThickness);
            drawList->AddLine({ center.x + kCrosshairGap,  center.y }, { center.x + kCrosshairSize, center.y }, kCrosshairColor, kCrosshairThickness);
            drawList->AddLine({ center.x, center.y - kCrosshairSize }, { center.x, center.y - kCrosshairGap  }, kCrosshairColor, kCrosshairThickness);
            drawList->AddLine({ center.x, center.y + kCrosshairGap  }, { center.x, center.y + kCrosshairSize }, kCrosshairColor, kCrosshairThickness);

            if (hitmarkerTimer > 0.0f)
            {
                constexpr float kHitmarkerLifetime = 0.18f;
                constexpr float kHitmarkerInner    = 5.0f;
                constexpr float kHitmarkerOuter    = 10.0f;
                constexpr float kHitmarkerThickness = 2.0f;
                const float alpha = std::clamp(hitmarkerTimer / kHitmarkerLifetime, 0.0f, 1.0f);
                const ImU32 color = IM_COL32(255, 255, 255, static_cast<int>(230.0f * alpha));
                // Four diagonal ticks at the crosshair corners (\ / / \).
                drawList->AddLine({ center.x - kHitmarkerOuter, center.y - kHitmarkerOuter }, { center.x - kHitmarkerInner, center.y - kHitmarkerInner }, color, kHitmarkerThickness);
                drawList->AddLine({ center.x + kHitmarkerInner, center.y - kHitmarkerInner }, { center.x + kHitmarkerOuter, center.y - kHitmarkerOuter }, color, kHitmarkerThickness);
                drawList->AddLine({ center.x - kHitmarkerOuter, center.y + kHitmarkerOuter }, { center.x - kHitmarkerInner, center.y + kHitmarkerInner }, color, kHitmarkerThickness);
                drawList->AddLine({ center.x + kHitmarkerInner, center.y + kHitmarkerInner }, { center.x + kHitmarkerOuter, center.y + kHitmarkerOuter }, color, kHitmarkerThickness);
                hitmarkerTimer -= deltaTime;
            }
        }
        imguiLayer.endFrame();

        window.swapBuffers();
    }

    return 0;
}
