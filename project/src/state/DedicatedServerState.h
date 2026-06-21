#pragma once

#include "core/GameState.h"
#include <string>

union SDL_Event;

// Server-only process: no rendering. Net::pump() does all the work;
// this state just keeps the window alive and prints status.
class DedicatedServerState : public GameState
{
public:
    explicit DedicatedServerState(Game& game);

    void enter() override;
    void handleEvent(const SDL_Event&) override {}
    void update(float dt, const bool* keys) override;
    void render()   override {}
    void renderUI() override;
    std::string name() const override { return "DedicatedServerState"; }
};
