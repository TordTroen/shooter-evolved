#include "NameGenerator.h"

#include <array>

namespace net
{
    namespace
    {
        constexpr std::array<std::string_view, 14> NAMES = {
            "Master Chief",  "Arbiter",   "Tartarus",   "Johnson",   "Grunt",   "Hunter",
            "Elite",   "Jackal", "343",   "Brute",  "Reclaimer",   "Caboose",
            "Griff",   "Frankie",
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
