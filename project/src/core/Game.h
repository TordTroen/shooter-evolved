#pragma once

#include "GameConfig.h"
#include "GameState.h"
#include "Window.h"
#include "../input/GamepadInput.h"
#include "../rendering/ModelLoader.h"
#include "../rendering/WeaponModelCache.h"
#include "../ui/ImGuiLayer.h"

#include <glm/glm.hpp>
#include <memory>

class Mesh;
class Net;
class Shader;
class Texture;

class Game
{
public:
    explicit Game(const GameConfig& cfg);
    ~Game();

    Game(const Game&)            = delete;
    Game& operator=(const Game&) = delete;

    void run();
    void requestState(std::unique_ptr<GameState> state);

    // Builds/replaces m_net for the given role. Call before transitioning to a
    // state that depends on networking (Lobby, Playing, DedicatedServer).
    void start_network(NetRole role, const std::string& host, uint16_t port);
    // Tears down the current network connection (if any) so a later
    // start_network() begins clean (GNS is ref-counted; destroying m_net
    // releases this side's reference).
    void stop_network();

    Window&                   window()              { return m_window; }
    ImGuiLayer&               imguiLayer()          { return m_imguiLayer; }
    GamepadInput&             gamepad()             { return m_gamepad; }
    Shader&                   shader()              { return *m_shader; }
    Mesh&                     planeMesh()           { return *m_planeMesh; }
    Mesh&                     boxMesh()             { return *m_boxMesh; }
    Texture&                  defaultWhiteTexture() { return *m_defaultWhiteTexture; }
    Texture&                  muzzleFlashTexture()  { return *m_muzzleFlashTexture; }
    Texture&                  decalTexture()        { return *m_decalTexture; }
    WeaponModelCache&         weaponModels()        { return m_weaponModels; }
    const glm::mat4&          projection()    const { return m_projection; }

    // nullptr in Solo mode.
    Net* net() { return m_net.get(); }

private:
    // Declaration order = construction order; window must be first.
    Window                   m_window;
    ImGuiLayer               m_imguiLayer;
    GamepadInput             m_gamepad;

    std::unique_ptr<Shader>  m_shader;
    std::unique_ptr<Mesh>    m_planeMesh;
    std::unique_ptr<Mesh>    m_boxMesh;
    std::unique_ptr<Texture> m_defaultWhiteTexture;
    std::unique_ptr<Texture> m_muzzleFlashTexture;
    std::unique_ptr<Texture> m_decalTexture;
    WeaponModelCache         m_weaponModels;

    glm::mat4                m_projection{ 1.0f };

    std::unique_ptr<Net>     m_net;

    std::unique_ptr<GameState> m_activeState;
    std::unique_ptr<GameState> m_pendingState;

    void applyPendingState();
};
