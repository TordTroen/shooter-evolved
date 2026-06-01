#pragma once

union SDL_Event;
class Game;

class GameState
{
public:
    explicit GameState(Game& game) : m_game(game) {}
    virtual ~GameState() = default;

    GameState(const GameState&)            = delete;
    GameState& operator=(const GameState&) = delete;

    virtual void enter() {}
    virtual void exit()  {}
    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void update(float dt, const bool* keys)  = 0;
    virtual void render()                            = 0;
    virtual void renderUI()                          = 0;

protected:
    Game& m_game;
};
