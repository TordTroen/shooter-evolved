#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <filesystem>
#include <iostream>
#include <algorithm>

#include "core/Window.h"
#include "rendering/GLDebug.h"
#include "rendering/Shader.h"
#include "rendering/Mesh.h"
#include "rendering/Vertex.h"
#include "scene/Camera.h"

// Walk up from the exe's directory until we find a folder containing "shaders/",
// then set that as the working directory. This makes relative asset paths work
// regardless of which directory the IDE or launcher uses.
static void setProjectRootAsWorkingDir() {
    namespace fs = std::filesystem;
    const char* exeDir = SDL_GetBasePath();
    if (!exeDir) return;

    fs::path dir = exeDir;
    while (dir.has_parent_path()) {
        if (fs::exists(dir / "shaders")) {
            fs::current_path(dir);
            std::cerr << "[Init] Working directory set to: " << dir << "\n";
            return;
        }
        fs::path parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    std::cerr << "[Init] WARNING: Could not find project root (no 'shaders/' found walking up from exe).\n";
}

// Unit quad in the XZ plane (y=0), normals pointing up.
// Scale X and Z via the model matrix to get the final size.
static constexpr std::array<Vertex, 4> kPlaneVerts = {{
    { {-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
    { { 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
    { { 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    { {-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
}};
static constexpr std::array<uint32_t, 6> kPlaneIdx = { 0, 1, 2,  0, 2, 3 };

// Unit box centred at origin (-0.5..0.5 on each axis).
// 4 unique vertices per face so each face has correct outward normals.
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

int main(int /*argc*/, char* /*argv*/[]) {
    setProjectRootAsWorkingDir();

    Window window({ .title = "FPS Demo", .width = 1280, .height = 720 });

    // Lock and hide the cursor for mouse look (SDL3 per-window API).
    SDL_SetWindowRelativeMouseMode(window.handle(), true);

    wireUpGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Mesh   planeMesh(kPlaneVerts, kPlaneIdx);
    Mesh   boxMesh(kBoxVerts, kBoxIdx);

    Camera camera(glm::vec3(0.0f, 1.7f, 8.0f));

    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        static_cast<float>(window.width()) / static_cast<float>(window.height()),
        0.1f, 1000.0f);

    // Skip the first mouse event — SDL may fire a large motion when the window
    // first gains focus, causing a camera snap.
    bool skipFirstMouseEvent = true;

    Uint64 lastTime = SDL_GetTicksNS();

    while (!window.shouldClose()) {
        const Uint64 now       = SDL_GetTicksNS();
        const float  deltaTime = std::min(static_cast<float>(now - lastTime) * 1e-9f, 0.05f);
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    window.setShouldClose(true);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                        window.setShouldClose(true);
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
                    if (skipFirstMouseEvent) { skipFirstMouseEvent = false; break; }
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
        camera.processKeyboard(keys, deltaTime);

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
