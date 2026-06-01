#include "core/Game.h"
#include "core/Paths.h"

int main(int /*argc*/, char* /*argv*/[])
{
    Paths::setWorkingDirToProjectRoot();
    Game game({ .title = "FPS Demo", .width = 1280, .height = 720 });
    game.run();
    return 0;
}
