#pragma once

#include "core/GameState.h"

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
};
