#include "Paths.h"

#include <SDL3/SDL.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace Paths
{
    bool setWorkingDirToProjectRoot(std::string_view anchor)
    {
        namespace fs = std::filesystem;
        const char* exeDir = SDL_GetBasePath();
        if (!exeDir)
            return false;

        fs::path dir = exeDir;
        while (dir.has_parent_path())
        {
            if (fs::exists(dir / std::string(anchor)))
            {
                fs::current_path(dir);
                std::cout << "[Init] Working directory set to: " << dir << "\n";
                return true;
            }
            fs::path parent = dir.parent_path();
            if (parent == dir)
                break;
            dir = parent;
        }
        std::cerr << "[Init] WARNING: Could not find project root (no '" << anchor
                  << "/' found walking up from exe).\n";
        return false;
    }
}
