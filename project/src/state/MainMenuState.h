#pragma once

#include "core/GameState.h"
#include <string>

union SDL_Event;

class MainMenuState : public GameState
{
public:
    explicit MainMenuState(Game& game);

    void enter() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt, const bool* keys) override;
    void render() override;
    void renderUI() override;
    std::string name() const override { return "MainMenuState"; }

private:
    enum class Page { Root, Host, Join };

    void renderRoot();
    void renderHost();
    void renderJoin();

    Page m_page = Page::Root;

    char m_hostPortBuf[16] = "7777";
    char m_joinIpBuf[64]   = "127.0.0.1";
    char m_joinPortBuf[16] = "7777";

    std::string m_error;
};
