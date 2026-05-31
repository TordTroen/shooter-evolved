#include "actor/Actor.h"
#include "actor/components/MeshRenderer.h"
#include "core/Paths.h"
#include "core/Window.h"
#include "player/CharacterController.h"
#include "player/Weapon.h"
#include "rendering/GLDebug.h"
#include "rendering/Mesh.h"
#include "rendering/ModelLoader.h"
#include "rendering/MuzzleFlashEffect.h"
#include "rendering/Primitives.h"
#include "rendering/ProceduralTextures.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "rendering/ViewmodelRenderer.h"
#include "scene/Camera.h"
#include "scene/DemoScene.h"
#include "ui/Hud.h"
#include "ui/ImGuiLayer.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>
#include <memory>

int main(int /*argc*/, char* /*argv*/[])
{
    std::cout << "[Init] Starting...\n";
    Paths::setWorkingDirToProjectRoot();
    Window window({ .title = "FPS Demo", .width = 1280, .height = 720 });
    SDL_SetWindowRelativeMouseMode(window.handle(), true);
    wireUpGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    ImGuiLayer imguiLayer(window);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Mesh   planeMesh(Primitives::planeVertices(), Primitives::planeIndices());
    Mesh   boxMesh(Primitives::boxVertices(), Primitives::boxIndices());
    constexpr uint8_t kWhitePixel[4] = { 255, 255, 255, 255 };
    Texture defaultWhiteTexture(kWhitePixel, 1, 1, 4);
    defaultWhiteTexture.bind(0);
    ModelLoader::LoadedModel gunModel = ModelLoader::loadGltf("assets/models/BasicGun.glb");
    constexpr int kMuzzleFlashTexSize = 128;
    const auto muzzleFlashPixels = ProceduralTextures::muzzleFlashRGBA(kMuzzleFlashTexSize);
    Texture muzzleFlashTexture(muzzleFlashPixels.data(), kMuzzleFlashTexSize, kMuzzleFlashTexSize, 4);
    DemoScene scene(planeMesh, boxMesh);
    scene.setup();

    std::unique_ptr<MeshRenderer>      gunMR;
    std::unique_ptr<ViewmodelRenderer> gunViewmodel;
    if (gunModel.mesh)
    {
        gunMR = std::make_unique<MeshRenderer>(gunModel.mesh.get(), glm::vec3(1.0f));
        gunMR->texture = gunModel.baseColorTexture.get();
        gunViewmodel   = std::make_unique<ViewmodelRenderer>(*gunMR);
    }
    MuzzleFlashEffect   muzzleFlash(planeMesh, muzzleFlashTexture);
    CharacterController character(scene.physics(), glm::vec3(0.0f, 2.0f, 8.0f));
    Camera              camera(glm::vec3(0.0f, 2.0f + CharacterController::eyeHeight(), 8.0f));
    glm::mat4           projection = glm::perspective(glm::radians(60.0f),
        static_cast<float>(window.width()) / static_cast<float>(window.height()), 0.1f, 1000.0f);
    bool   skipFirstMouseEvent = true;
    bool   shouldFire          = false;
    Weapon weapon;
    Hud    hud;
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
                    window.setShouldClose(true); break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) window.setShouldClose(true);
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    window.onResize(event.window.data1, event.window.data2);
                    projection = glm::perspective(glm::radians(60.0f),
                        static_cast<float>(event.window.data1) / static_cast<float>(event.window.data2),
                        0.1f, 1000.0f);
                    glViewport(0, 0, event.window.data1, event.window.data2);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) shouldFire = true;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (skipFirstMouseEvent) { skipFirstMouseEvent = false; break; }
                    camera.processMouseMotion(
                        static_cast<float>(event.motion.xrel),
                        static_cast<float>(event.motion.yrel));
                    break;
                default: break;
            }
        }
        int numKeys = 0;
        const bool* keys = SDL_GetKeyboardState(&numKeys);
        if (shouldFire)
        {
            shouldFire = false;
            muzzleFlash.trigger();
            const FireResult result = weapon.fire(scene,
                character.position() + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f),
                camera.front());
            if (result.damaged) hud.triggerHitmarker();
        }

        scene.tick(deltaTime);
        character.update(deltaTime, keys, camera.front());
        camera.setPosition(character.position()
            + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f));
        shader.checkHotReload();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        shader.setMat4("view",       camera.getViewMatrix());
        shader.setMat4("projection", projection);

        for (const auto& actor : scene.actors())
            if (actor->meshRenderer)
                actor->meshRenderer->draw(shader, actor->modelMatrix());

        if (gunViewmodel)
        {
            gunViewmodel->render(shader, camera);
            muzzleFlash.render(shader, gunViewmodel->muzzleWorldPos(camera, 0.65f),
                camera.right(), camera.up(), -camera.front(), deltaTime);
        }

        imguiLayer.beginFrame();
        hud.draw(deltaTime);
        imguiLayer.endFrame();
        window.swapBuffers();
    }

    return 0;
}
