#include "MainMenuState.h"
#include "PlayingState.h"
#include "core/Game.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <memory>

MainMenuState::MainMenuState(Game& game)
    : GameState(game)
{}

void MainMenuState::enter()
{
    SDL_SetWindowRelativeMouseMode(m_game.window().handle(), false);
}

void MainMenuState::handleEvent(const SDL_Event& /*event*/)
{}

void MainMenuState::update(float /*dt*/, const bool* /*keys*/)
{}

void MainMenuState::render()
{}

void MainMenuState::renderUI()
{
    const ImGuiIO& io = ImGui::GetIO();
    constexpr float kBtnW = 200.0f;
    constexpr float kBtnH = 60.0f;
    ImGui::SetNextWindowPos({ (io.DisplaySize.x - kBtnW) * 0.5f,
                              (io.DisplaySize.y - kBtnH) * 0.5f });
    ImGui::SetNextWindowSize({ kBtnW, kBtnH });
    ImGui::Begin("##menu", nullptr,
        ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground);
    if (ImGui::Button("Start", { kBtnW, kBtnH }))
        m_game.requestState(std::make_unique<PlayingState>(m_game));
    ImGui::End();
}
