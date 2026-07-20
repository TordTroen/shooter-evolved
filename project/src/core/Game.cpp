#include "Game.h"
#include "Paths.h"
#include "../net/Net.h"
#include "../rendering/GLDebug.h"
#include "../rendering/Mesh.h"
#include "../rendering/Primitives.h"
#include "../rendering/ProceduralTextures.h"
#include "../rendering/Shader.h"
#include "../rendering/Texture.h"
#include "../state/LobbyState.h"
#include "../state/MainMenuState.h"
#include "../state/PlayingState.h"
#include "../weapons/WeaponRegistry.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>

Game::Game(const GameConfig& cfg)
    : m_window({ .title = cfg.title.c_str(), .width = cfg.width, .height = cfg.height })
    , m_imguiLayer(m_window)
{
    std::cout << "Initializing game\n";
    wireUpGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    m_shader    = std::make_unique<Shader>("shaders/basic.vert", "shaders/basic.frag");
    m_planeMesh = std::make_unique<Mesh>(Primitives::planeVertices(), Primitives::planeIndices());
    m_boxMesh   = std::make_unique<Mesh>(Primitives::boxVertices(), Primitives::boxIndices());

    constexpr uint8_t kWhitePixel[4] = { 255, 255, 255, 255 };
    m_defaultWhiteTexture = std::make_unique<Texture>(kWhitePixel, 1, 1, 4);
    m_defaultWhiteTexture->bind(0);

    m_weaponModels.load();

    constexpr int kMuzzleTexSize    = 128;
    const auto    muzzleFlashPixels = ProceduralTextures::muzzleFlashRGBA(kMuzzleTexSize);
    m_muzzleFlashTexture = std::make_unique<Texture>(
        muzzleFlashPixels.data(), kMuzzleTexSize, kMuzzleTexSize, 4);

    constexpr int kDecalTexSize = 64;
    const auto    decalPixels   = ProceduralTextures::bulletHoleRGBA(kDecalTexSize);
    m_decalTexture = std::make_unique<Texture>(
        decalPixels.data(), kDecalTexSize, kDecalTexSize, 4);

    m_projection = glm::perspective(
        glm::radians(60.0f),
        static_cast<float>(m_window.width()) / static_cast<float>(m_window.height()),
        0.1f, 1000.0f);

    // Only build Net (and skip the menu) when a role was explicitly requested
    // on the command line; otherwise start Net-less in MainMenuState.
    const auto& netCfg = cfg.net;
    if (netCfg.role == NetRole::Dedicated)
    {
        // --dedicated is served by the graphics-free fps_server executable now;
        // Game always owns a window, so it never builds a Dedicated Net.
        std::cerr << "[Game] --dedicated is not supported by fps_demo; "
                     "run fps_server.exe instead.\n";
        m_activeState = std::make_unique<MainMenuState>(*this);
    }
    else if (netCfg.hasCliRole)
    {
        start_network(netCfg.role, netCfg.host, netCfg.port);

        switch (netCfg.role)
        {
            case NetRole::Solo:
                m_activeState = std::make_unique<PlayingState>(*this);
                break;
            case NetRole::Host:
            case NetRole::Client:
                m_activeState = std::make_unique<LobbyState>(*this);
                break;
            case NetRole::Dedicated:
                break; // unreachable: handled above
        }
    }
    else
    {
        m_activeState = std::make_unique<MainMenuState>(*this);
    }
    std::cout << "Starting initial game state " << m_activeState->name() << "\n";
    m_activeState->enter();
}

Game::~Game() = default;

void Game::start_network(NetRole role, const std::string& host, uint16_t port)
{
    m_net = std::make_unique<Net>(role, host, port);
}

void Game::stop_network()
{
    m_net.reset();
}

void Game::requestState(std::unique_ptr<GameState> state)
{
    m_pendingState = std::move(state);
}

void Game::applyPendingState()
{
    if (!m_pendingState) return;
    if (m_activeState) m_activeState->exit();
    m_activeState = std::move(m_pendingState);
    m_activeState->enter();
}

void Game::run()
{
    Uint64 lastTime = SDL_GetTicksNS();
    while (!m_window.shouldClose())
    {
        const Uint64 now       = SDL_GetTicksNS();
        const float  deltaTime = std::min(static_cast<float>(now - lastTime) * 1e-9f, 0.05f);
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            m_imguiLayer.processEvent(event);
            m_gamepad.handleEvent(event);
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    m_window.setShouldClose(true); break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                        m_window.setShouldClose(true);
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    m_window.onResize(event.window.data1, event.window.data2);
                    m_projection = glm::perspective(glm::radians(60.0f),
                        static_cast<float>(event.window.data1) /
                        static_cast<float>(event.window.data2),
                        0.1f, 1000.0f);
                    glViewport(0, 0, event.window.data1, event.window.data2);
                    break;
                default: break;
            }
            if (m_activeState)
                m_activeState->handleEvent(event);
        }

        int numKeys = 0;
        const bool* keys = SDL_GetKeyboardState(&numKeys);
        m_gamepad.update(deltaTime);

        // Pump network before state update so messages are processed at a
        // consistent point regardless of which state is active.
        if (m_net) m_net->pump(deltaTime);

        if (m_activeState)
        {
            m_activeState->update(deltaTime, keys);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_activeState->render();
            m_imguiLayer.beginFrame();
            m_activeState->renderUI();
            m_imguiLayer.endFrame();
        }

        m_window.swapBuffers();
        applyPendingState();
    }
}
