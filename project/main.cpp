#include "core/Args.h"
#include "core/Game.h"
#include "core/Paths.h"

#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << std::unitbuf;  // flush after every <<
    Paths::setWorkingDirToProjectRoot();
    const GameConfig cfg = parseArgs(argc, argv);
    Game game(cfg);
    game.run();
    return 0;
}
