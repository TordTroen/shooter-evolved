#pragma once
#include <string_view>

namespace Paths
{
    // Walks up from the exe directory until it finds a subdirectory named
    // `anchor`, then sets that directory as the working directory.
    // Returns true on success, false if not found or SDL_GetBasePath failed.
    bool setWorkingDirToProjectRoot(std::string_view anchor = "shaders");
}
