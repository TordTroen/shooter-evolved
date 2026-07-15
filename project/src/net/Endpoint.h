#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace net
{

struct Endpoint
{
    std::string host;
    uint16_t    port = 0;
};

// Parses a "host:port" or bare "port" string into an Endpoint.
// - `text` with a colon: splits into host/port ("host:port").
// - `text` without a colon: treated as a bare port, host left empty.
// Rejects non-numeric, missing, zero, or out-of-range (>65535) ports.
// Pure function, no I/O - safe to unit test directly.
std::optional<Endpoint> parse_endpoint(const std::string& text);

// Parses a bare port string (e.g. from a menu's port field).
// Rejects non-numeric, missing, zero, or out-of-range (>65535) ports.
std::optional<uint16_t> parse_port(const std::string& text);

} // namespace net
