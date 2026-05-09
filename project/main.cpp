#include "core/Window.h"
#include "physics/CharacterController.h"
#include "physics/PhysicsLayers.h"
#include "physics/PhysicsWorld.h"
#include "rendering/GLDebug.h"
#include "rendering/Mesh.h"
#include "rendering/Shader.h"
#include "rendering/Vertex.h"
#include "scene/Camera.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>

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
static constexpr std::array<uint32_t, 36> kBoxIdx = {
     0,  1,  2,   0,  2,  3,   // +X
     4,  5,  6,   4,  6,  7,   // -X
     8,  9, 10,   8, 10, 11,   // +Y
    12, 13, 14,  12, 14, 15,   // -Y
    16, 17, 18,  16, 18, 19,   // +Z
    20, 21, 22,  20, 22, 23,   // -Z
};

static void addStaticBox(PhysicsWorld& physics, JPH::Vec3 center, JPH::Vec3 halfExtents)
{
    JPH::BodyCreationSettings settings(
        new JPH::BoxShape(halfExtents),
        JPH::RVec3(center),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING);
    physics.system().GetBodyInterface().CreateAndAddBody(
        settings, JPH::EActivation::DontActivate);
}

int main(int /*argc*/, char* /*argv*/[])
{
    setProjectRootAsWorkingDir();

    PhysicsWorld physics;

    // --- Static level geometry (must match the rendered scene) ---
    // Plane: 20×20 quad rendered at y=0. Physics: thin box spanning the ground.
    addStaticBox(physics, JPH::Vec3(0.0f, -0.5f, 0.0f), JPH::Vec3(10.0f, 0.5f, 10.0f));
    // Box 1: unit cube, centre at (−3, 0.5, −5)
    addStaticBox(physics, JPH::Vec3(-3.0f, 0.5f, -5.0f), JPH::Vec3(0.5f, 0.5f, 0.5f));
    // Box 2: 1×2×1, centre at (3, 1, −8)
    addStaticBox(physics, JPH::Vec3(3.0f, 1.0f, -8.0f), JPH::Vec3(0.5f, 1.0f, 0.5f));

    // Optimise the broadphase after adding all static bodies.
    physics.system().OptimizeBroadPhase();

    // Player starts above the plane so we can verify gravity.
    CharacterController character(physics, glm::vec3(0.0f, 2.0f, 8.0f));

    Window window({ .title = "FPS Demo", .width = 1280, .height = 720 });

    SDL_SetWindowRelativeMouseMode(window.handle(), true);

    wireUpGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Mesh   planeMesh(kPlaneVerts, kPlaneIdx);
    Mesh   boxMesh(kBoxVerts, kBoxIdx);

    // Camera starts at the character's eye position.
    const glm::vec3 startFeet(0.0f, 2.0f, 8.0f);
    Camera camera(startFeet + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f));

    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        static_cast<float>(window.width()) / static_cast<float>(window.height()),
        0.1f, 1000.0f);

    bool skipFirstMouseEvent = true;

    Uint64 lastTime = SDL_GetTicksNS();

    while (!window.shouldClose())
    {
        const Uint64 now       = SDL_GetTicksNS();
        const float  deltaTime = std::min(static_cast<float>(now - lastTime) * 1e-9f, 0.05f);
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
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

        // Compute desired horizontal velocity from WASD + camera look direction.
        constexpr float kMoveSpeed = 5.0f;
        const glm::vec3 wishVel = camera.getWishVelocity(keys, kMoveSpeed);

        // Step physics, then update character controller.
        physics.update(deltaTime);
        character.update(deltaTime, wishVel);

        // Sync camera to character eye position.
        const glm::vec3 feet = character.position();
        camera.setPosition(feet + glm::vec3(0.0f, CharacterController::eyeHeight(), 0.0f));

        shader.checkHotReload();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("view",       camera.getViewMatrix());
        shader.setMat4("projection", projection);

        // Plane — scaled 20×20 in XZ, sitting at y=0
        glm::mat4 model = glm::scale(glm::mat4(1.0f), {20.0f, 1.0f, 20.0f});
        shader.setMat4("model", model);
        planeMesh.draw();

        // Box 1 — unit cube, bottom at y=0
        model = glm::translate(glm::mat4(1.0f), {-3.0f, 0.5f, -5.0f});
        shader.setMat4("model", model);
        boxMesh.draw();

        // Box 2 — tall box (2 units high), bottom at y=0
        model = glm::translate(glm::mat4(1.0f), {3.0f, 1.0f, -8.0f});
        model = glm::scale(model, {1.0f, 2.0f, 1.0f});
        shader.setMat4("model", model);
        boxMesh.draw();

        window.swapBuffers();
    }

    return 0;
}
