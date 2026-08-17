#include "core/Args.h"
#include "core/Game.h"
#include "core/Paths.h"

#include <cstdio>
#include <print>

int main(int argc, char* argv[])
{
    std::setvbuf(stdout, nullptr, _IONBF, 0); // flush after every print, for a live-tailed log
    Paths::setWorkingDirToProjectRoot();
    const GameConfig cfg = parseArgs(argc, argv);
    std::println("Starting game");
    Game game(cfg);
    game.run();
    return 0;
}
