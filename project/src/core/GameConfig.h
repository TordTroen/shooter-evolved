#pragma once

#include "../net/NetRole.h"

#include <cstdint>
#include <string>

struct NetConfig
{
    NetRole     role       = NetRole::Solo;
    std::string host       = "127.0.0.1";
    uint16_t    port       = 7777;
    bool        hasCliRole = false; // true when a role was set from CLI args
};

struct GameConfig
{
    std::string title  = "FPS Demo";
    int         width  = 1280;
    int         height = 720;
    NetConfig   net;
};
