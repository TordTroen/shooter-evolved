#pragma once
#include <cstddef>
#include <string_view>

namespace net
{
    // Number of hardcoded names available.
    size_t name_count();

    // Name at index, wrapping modulo name_count(). Pure function → unit-testable.
    std::string_view player_name_at(size_t index);
}
