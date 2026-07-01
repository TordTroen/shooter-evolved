#include "Endpoint.h"

#include <charconv>

namespace net
{

std::optional<uint16_t> parse_port(const std::string& text)
{
    if (text.empty()) return std::nullopt;

    unsigned long value  = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) return std::nullopt;
    if (value == 0 || value > 65535) return std::nullopt;

    return static_cast<uint16_t>(value);
}

std::optional<Endpoint> parse_endpoint(const std::string& text)
{
    const auto colon = text.rfind(':');
    if (colon == std::string::npos)
    {
        const auto port = parse_port(text);
        if (!port) return std::nullopt;
        return Endpoint{ "", *port };
    }

    const std::string host     = text.substr(0, colon);
    const std::string portText = text.substr(colon + 1);
    if (host.empty()) return std::nullopt;

    const auto port = parse_port(portText);
    if (!port) return std::nullopt;

    return Endpoint{ host, *port };
}

} // namespace net
