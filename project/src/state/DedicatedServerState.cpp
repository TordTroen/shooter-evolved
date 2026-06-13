#include "DedicatedServerState.h"
#include "core/Game.h"
#include "net/Net.h"
#include "net/Server.h"

#include <imgui.h>
#include <iostream>

DedicatedServerState::DedicatedServerState(Game& game)
    : GameState(game)
{
}

void DedicatedServerState::enter()
{
    std::cout << "[Dedicated] Server running. Press Escape to quit.\n";
}

void DedicatedServerState::update(float /*dt*/, const bool* /*keys*/)
{
    // Net::pump() in Game::run() ticks the server; nothing else to do here.
}

void DedicatedServerState::renderUI()
{
    ImGui::SetNextWindowPos(ImVec2(40, 40));
    ImGui::Begin("Dedicated Server", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);
    ImGui::Text("Running. Press Escape to quit.");
    ImGui::End();
}
