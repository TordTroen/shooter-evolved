#pragma once

#include "core/GameState.h"

union SDL_Event;

// Multiplayer pre-game state. Host waits for players; clients wait for the
// server's StartGame signal. Transitions to PlayingState on start.
class LobbyState : public GameState
{
public:
    explicit LobbyState(Game& game);

    void enter()  override;
    void exit()   override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt, const bool* keys) override;
    void render()   override {}
    void renderUI() override;

private:
    bool     m_startRequested  = false; // host pressed Enter
    bool     m_serverSaidStart = false; // server broadcast StartGame
};
