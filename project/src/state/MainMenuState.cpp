#include "MainMenuState.h"
#include "LobbyState.h"
#include "core/Game.h"
#include "net/Endpoint.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <cstring>
#include <memory>

MainMenuState::MainMenuState(Game& game)
    : GameState(game)
{}

void MainMenuState::enter()
{
    SDL_SetWindowRelativeMouseMode(m_game.window().handle(), false);
    m_page = Page::Root;
    m_error.clear();
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
    constexpr float kWinW = 320.0f;
    constexpr float kWinH = 220.0f;
    ImGui::SetNextWindowPos({ (io.DisplaySize.x - kWinW) * 0.5f,
                              (io.DisplaySize.y - kWinH) * 0.5f });
    ImGui::SetNextWindowSize({ kWinW, kWinH });
    ImGui::Begin("##menu", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground);

    switch (m_page)
    {
        case Page::Root: renderRoot(); break;
        case Page::Host: renderHost(); break;
        case Page::Join: renderJoin(); break;
    }

    ImGui::End();
}

void MainMenuState::renderRoot()
{
    constexpr float kBtnW = 200.0f;
    constexpr float kBtnH = 50.0f;

    if (ImGui::Button("Host", { kBtnW, kBtnH }))
    {
        m_page = Page::Host;
        m_error.clear();
    }
    ImGui::Spacing();
    if (ImGui::Button("Join", { kBtnW, kBtnH }))
    {
        m_page = Page::Join;
        m_error.clear();
    }
    ImGui::Spacing();
    if (ImGui::Button("Quit", { kBtnW, kBtnH }))
    {
        m_game.window().setShouldClose(true);
    }
}

void MainMenuState::renderHost()
{
    ImGui::Text("Host Game");
    ImGui::Spacing();
    ImGui::Text("Port");
    ImGui::InputText("##hostPort", m_hostPortBuf, sizeof(m_hostPortBuf),
        ImGuiInputTextFlags_CharsDecimal);
    ImGui::Spacing();

    if (ImGui::Button("Start Hosting", { 150.0f, 40.0f }))
    {
        const auto port = net::parse_port(m_hostPortBuf);
        if (!port)
        {
            m_error = "Invalid port";
        }
        else
        {
            m_game.start_network(NetRole::Host, "0.0.0.0", *port);
            m_game.requestState(std::make_unique<LobbyState>(m_game));
            return;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Back", { 100.0f, 40.0f }))
    {
        m_page = Page::Root;
        m_error.clear();
    }

    if (!m_error.empty())
    {
        ImGui::TextColored({ 1.0f, 0.3f, 0.3f, 1.0f }, "%s", m_error.c_str());
    }
}

void MainMenuState::renderJoin()
{
    ImGui::Text("Join Game");
    ImGui::Spacing();
    ImGui::Text("IP Address");
    ImGui::InputText("##joinIp", m_joinIpBuf, sizeof(m_joinIpBuf));
    ImGui::Text("Port");
    ImGui::InputText("##joinPort", m_joinPortBuf, sizeof(m_joinPortBuf),
        ImGuiInputTextFlags_CharsDecimal);
    ImGui::Spacing();

    if (ImGui::Button("Connect", { 150.0f, 40.0f }))
    {
        const auto port = net::parse_port(m_joinPortBuf);
        if (std::strlen(m_joinIpBuf) == 0)
        {
            m_error = "Invalid IP address";
        }
        else if (!port)
        {
            m_error = "Invalid port";
        }
        else
        {
            m_game.start_network(NetRole::Client, m_joinIpBuf, *port);
            m_game.requestState(std::make_unique<LobbyState>(m_game));
            return;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Back", { 100.0f, 40.0f }))
    {
        m_page = Page::Root;
        m_error.clear();
    }

    if (!m_error.empty())
    {
        ImGui::TextColored({ 1.0f, 0.3f, 0.3f, 1.0f }, "%s", m_error.c_str());
    }
}
