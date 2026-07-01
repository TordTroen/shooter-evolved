#include "Args.h"

#include <cstdlib>
#include <iostream>
#include <string>

GameConfig parseArgs(int argc, char* argv[])
{
    GameConfig cfg;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "--host")
        {
            cfg.net.role       = NetRole::Host;
            cfg.net.hasCliRole = true;
        }
        else if (arg == "--dedicated")
        {
            cfg.net.role       = NetRole::Dedicated;
            cfg.net.hasCliRole = true;
        }
        else if (arg == "--solo")
        {
            cfg.net.role       = NetRole::Solo;
            cfg.net.hasCliRole = true;
        }
        else if (arg == "--connect" && i + 1 < argc)
        {
            cfg.net.role       = NetRole::Client;
            cfg.net.hasCliRole = true;
            std::string addr = argv[++i];
            // Accept "host:port" or bare host.
            const auto colon = addr.rfind(':');
            if (colon != std::string::npos)
            {
                cfg.net.host = addr.substr(0, colon);
                cfg.net.port = static_cast<uint16_t>(std::atoi(addr.c_str() + colon + 1));
            }
            else
            {
                cfg.net.host = addr;
            }
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            cfg.net.port = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else
        {
            std::cerr << "[Args] Unknown argument: " << arg << "\n";
        }
    }

    return cfg;
}
