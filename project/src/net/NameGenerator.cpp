#include "NameGenerator.h"

#include <array>

namespace net
{
    namespace
    {
        constexpr std::array<std::string_view, 24> NAMES = {
            "Falcon",  "Viper",   "Ghost",   "Raven",   "Cobra",   "Hunter",
            "Nomad",   "Phantom", "Rogue",   "Wraith",  "Titan",   "Reaper",
            "Blaze",   "Shadow",  "Storm",   "Talon",   "Fox",     "Wolf",
            "Havoc",   "Specter", "Recon",   "Drifter", "Marauder","Jackal",
        };
    }

    size_t name_count()
    {
        return NAMES.size();
    }

    std::string_view player_name_at(size_t index)
    {
        return NAMES[index % NAMES.size()];
    }
}
