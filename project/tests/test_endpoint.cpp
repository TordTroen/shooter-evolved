#include <catch2/catch_test_macros.hpp>

#include "net/Endpoint.h"

// ---- parse_port ----

TEST_CASE("parse_port: valid port")
{
    const auto port = net::parse_port("7777");
    REQUIRE(port.has_value());
    REQUIRE(*port == 7777);
}

TEST_CASE("parse_port: rejects zero")
{
    REQUIRE_FALSE(net::parse_port("0").has_value());
}

TEST_CASE("parse_port: rejects out-of-range")
{
    REQUIRE_FALSE(net::parse_port("65536").has_value());
    REQUIRE_FALSE(net::parse_port("999999").has_value());
}

TEST_CASE("parse_port: accepts max valid value")
{
    const auto port = net::parse_port("65535");
    REQUIRE(port.has_value());
    REQUIRE(*port == 65535);
}

TEST_CASE("parse_port: rejects missing value")
{
    REQUIRE_FALSE(net::parse_port("").has_value());
}

TEST_CASE("parse_port: rejects junk")
{
    REQUIRE_FALSE(net::parse_port("abc").has_value());
    REQUIRE_FALSE(net::parse_port("12ab").has_value());
    REQUIRE_FALSE(net::parse_port("-1").has_value());
    REQUIRE_FALSE(net::parse_port("12.5").has_value());
}

// ---- parse_endpoint ----

TEST_CASE("parse_endpoint: host:port")
{
    const auto ep = net::parse_endpoint("192.168.1.42:7777");
    REQUIRE(ep.has_value());
    REQUIRE(ep->host == "192.168.1.42");
    REQUIRE(ep->port == 7777);
}

TEST_CASE("parse_endpoint: bare port, no host")
{
    const auto ep = net::parse_endpoint("7777");
    REQUIRE(ep.has_value());
    REQUIRE(ep->host.empty());
    REQUIRE(ep->port == 7777);
}

TEST_CASE("parse_endpoint: rejects missing port after colon")
{
    REQUIRE_FALSE(net::parse_endpoint("192.168.1.42:").has_value());
}

TEST_CASE("parse_endpoint: rejects empty host before colon")
{
    REQUIRE_FALSE(net::parse_endpoint(":7777").has_value());
}

TEST_CASE("parse_endpoint: rejects out-of-range port")
{
    REQUIRE_FALSE(net::parse_endpoint("192.168.1.42:70000").has_value());
}

TEST_CASE("parse_endpoint: rejects junk port")
{
    REQUIRE_FALSE(net::parse_endpoint("192.168.1.42:junk").has_value());
}
