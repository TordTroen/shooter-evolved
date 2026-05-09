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

// A simple triangle to verify the MVP pipeline.
static constexpr std::array<Vertex, 3> kTriVerts = {{
    { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
    { { 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
    { { 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f} },
}};
static constexpr std::array<uint32_t, 3> kTriIdx = { 0, 1, 2 };

int main(int /*argc*/, char* /*argv*/[]) {
    setProjectRootAsWorkingDir();

    Window window({ .title = "FPS Demo", .width = 1280, .height = 720 });

    // Lock and hide the cursor for mouse look (SDL3 per-window API).
    SDL_SetWindowRelativeMouseMode(window.handle(), true);

    wireUpGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Mesh   triMesh(kTriVerts, kTriIdx);

    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

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

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        triMesh.draw();

        window.swapBuffers();
    }

    return 0;
}
