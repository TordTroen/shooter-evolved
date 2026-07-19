#include "Paths.h"

#include <filesystem>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#else
#include <climits>
#include <unistd.h>
#endif

namespace
{
    // Returns the directory containing the running executable, or an empty
    // path on failure.
    std::filesystem::path exeDir()
    {
        namespace fs = std::filesystem;
#if defined(_WIN32)
        wchar_t buffer[MAX_PATH];
        const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
            return {};
        return fs::path(buffer, buffer + length).parent_path();
#else
        char buffer[PATH_MAX];
        const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (length <= 0)
            return {};
        return fs::path(std::string(buffer, static_cast<size_t>(length))).parent_path();
#endif
    }
}

namespace Paths
{
    bool setWorkingDirToProjectRoot(std::string_view anchor)
    {
        namespace fs = std::filesystem;
        fs::path dir = exeDir();
        if (dir.empty())
            return false;

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
