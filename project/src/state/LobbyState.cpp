#include "LobbyState.h"
#include "PlayingState.h"
#include "core/Game.h"
#include "net/Net.h"
#include "net/Client.h"
#include "net/Server.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <iostream>

LobbyState::LobbyState(Game& game)
    : GameState(game)
{
}

void LobbyState::enter()
{
    Net* net = m_game.net();
    if (!net) return;

    // When client receives StartGame, transition.
    if (Client* client = net->client())
    {
        client->onStartGame = [this]() {
            m_serverSaidStart = true;
        };
    }
}

void LobbyState::exit()
{
    // Clear callbacks so they don't fire after state teardown.
    Net* net = m_game.net();
    if (net && net->client())
        net->client()->onStartGame = nullptr;
}

void LobbyState::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_RETURN)
    {
        m_startRequested = true;
    }
}

void LobbyState::update(float /*dt*/, const bool* /*keys*/)
{
    Net* net = m_game.net();

    // Host: on Enter press, broadcast StartGame; everyone (including host's local
    // client via GNS localhost) will receive it and transition.
    if (m_startRequested && net && net->server())
    {
        m_startRequested = false;
        std::cout << "[Lobby] Host broadcasting StartGame\n";
        net->server()->broadcastStartGame();
        // Host has no local client in Dedicated mode → transition directly.
        if (!net->client())
            m_game.requestState(std::make_unique<PlayingState>(m_game));
        return;
    }

    // Client: transition when server said go.
    if (m_serverSaidStart)
    {
        m_serverSaidStart = false;
        m_game.requestState(std::make_unique<PlayingState>(m_game));
    }
}

void LobbyState::renderUI()
{
    ImGui::SetNextWindowPos(ImVec2(40, 40));
    ImGui::Begin("Lobby", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);

    Net* net = m_game.net();
    Client* client = net ? net->client() : nullptr;

    if (net && net->role() == NetRole::Host)
    {
        ImGui::Text("You are the host.");
    }
    else
    {
        ImGui::Text("Connecting to server...");
    }

    ImGui::Spacing();
    ImGui::Text("Players:");
    ImGui::Separator();
    if (client)
    {
        const NetworkId localId = client->localPlayerId();
        for (const auto& entry : client->roster())
        {
            const bool is_local = (entry.netId == localId);
            ImGui::Text("%s%s", entry.name.c_str(), is_local ? " (you)" : "");
        }
    }
    ImGui::Spacing();

    if (net && net->role() == NetRole::Host)
    {
        ImGui::Text("Press Enter to start.");
    }
    else if (client && client->localPlayerId())
    {
        ImGui::Text("Waiting for host to start.");
    }
    ImGui::End();
}
