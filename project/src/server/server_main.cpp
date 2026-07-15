#include "HeadlessServer.h"
#include "../core/Args.h"
#include "../core/Paths.h"

#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << std::unitbuf; // flush after every <<
    Paths::setWorkingDirToProjectRoot();
    GameConfig cfg = parseArgs(argc, argv);
    cfg.net.role   = NetRole::Dedicated; // fps_server has no other purpose

    std::cout << "Starting dedicated server\n";
    HeadlessServer server(cfg.net);
    server.run();
    return 0;
}
