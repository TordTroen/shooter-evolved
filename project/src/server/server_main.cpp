#include "HeadlessServer.h"
#include "../core/Args.h"
#include "../core/Paths.h"

#include <cstdio>
#include <print>

int main(int argc, char* argv[])
{
    std::setvbuf(stdout, nullptr, _IONBF, 0); // flush after every print, for a live-tailed log
    Paths::setWorkingDirToProjectRoot();
    GameConfig cfg = parseArgs(argc, argv);
    cfg.net.role   = NetRole::Dedicated; // fps_server has no other purpose

    std::println("Starting dedicated server");
    HeadlessServer server(cfg.net);
    server.run();
    return 0;
}
