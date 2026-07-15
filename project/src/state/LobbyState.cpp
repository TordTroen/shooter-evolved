#include "LobbyState.h"
#include "MainMenuState.h"
#include "PlayingState.h"
#include "core/Game.h"
#include "net/Net.h"
#include "net/Client.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <iostream>
#include <memory>

LobbyState::LobbyState(Game& game)
    : GameState(game)
{
}

void LobbyState::enter()
{
    Net* net = m_game.net();
    if (!net) return;

    // When client receives StartGame, transition. A disconnect (never
    // connected, or dropped after connecting) shows a failure message instead.
    if (Client* client = net->client())
    {
        client->onStartGame = [this]() {
            m_serverSaidStart = true;
        };
        client->onDisconnected = [this]() {
            m_connectionFailed = true;
        };
    }
}

void LobbyState::exit()
{
    // Clear callbacks so they don't fire after state teardown.
    Net* net = m_game.net();
    if (net && net->client())
    {
        net->client()->onStartGame    = nullptr;
        net->client()->onDisconnected = nullptr;
    }
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

    // A failed/lost connection takes priority; renderUI() shows the failure
    // message and a "Back to Menu" button that performs the actual transition.
    if (m_connectionFailed || (net && net->client() &&
        net->client()->connectionState() == ConnState::Failed))
    {
        m_connectionFailed = true;
        return;
    }

    // On Enter press, the local player asks the server to start (leader-gated on the
    // server; non-leaders are silently dropped). Same path for Host and Client - the
    // host always has a local loopback Client (see Net.cpp), so there is no
    // server-only shortcut here (NetworkingGuidelines §1).
    if (m_startRequested && net && net->client())
    {
        m_startRequested = false;
        net->client()->requestStartGame();
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

    if (m_connectionFailed)
    {
        ImGui::TextColored({ 1.0f, 0.3f, 0.3f, 1.0f }, "Failed to connect / connection lost");
        ImGui::Spacing();
        if (ImGui::Button("Back to Menu", { 150.0f, 40.0f }))
        {
            m_game.stop_network();
            m_game.requestState(std::make_unique<MainMenuState>(m_game));
        }
        ImGui::End();
        return;
    }

    if (net)
    {
        if (net->role() == NetRole::Host)
        {
            ImGui::Text("You are the host.");
        }
        else if (client->isLeader())
        {
            ImGui::Text("You are the party leader.");
        }
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

    ImGui::Separator();
    ImGui::Spacing();

    if (client && client->localPlayerId() && client->isLeader())
    {
        ImGui::Text("Press Enter to start.");
    }
    else if (client && client->localPlayerId())
    {
        const std::string leaderName = [&]() -> std::string {
            for (const auto& entry : client->roster())
            {
                if (entry.netId == client->leaderId())
                    return entry.name;
            }
            return "the leader";
        }();
        ImGui::Text("Waiting for %s to start.", leaderName.c_str());
    }
    ImGui::End();
}
