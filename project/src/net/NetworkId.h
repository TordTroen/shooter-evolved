#pragma once
#include <cstdint>
#include <functional>

struct NetworkId
{
    uint32_t value = 0;

    bool operator==(const NetworkId& o) const { return value == o.value; }
    bool operator!=(const NetworkId& o) const { return value != o.value; }
    explicit operator bool() const { return value != 0; }
};

static constexpr NetworkId kInvalidNetworkId{};

namespace std
{
    template<> struct hash<NetworkId>
    {
        size_t operator()(const NetworkId& id) const noexcept
        {
            return hash<uint32_t>{}(id.value);
        }
    };
}
