#include <catch2/catch_test_macros.hpp>

#include "core/Args.h"
#include "net/NetRole.h"

#include <vector>
#include <string>

// Builds a null-terminated argv from a list of string literals.
// argv[0] is always a fake program name.
static GameConfig parse(std::vector<const char*> args)
{
    args.insert(args.begin(), "fps-demo");
    return parseArgs(static_cast<int>(args.size()),
                     const_cast<char**>(args.data()));
}

// ---- defaults ----

TEST_CASE("parseArgs: no args -> Solo with defaults")
{
    const GameConfig cfg = parse({});
    REQUIRE(cfg.net.role == NetRole::Solo);
    REQUIRE(cfg.net.host == "127.0.0.1");
    REQUIRE(cfg.net.port == 7777);
}

// ---- --host ----

TEST_CASE("parseArgs: --host -> Host role")
{
    const GameConfig cfg = parse({"--host"});
    REQUIRE(cfg.net.role == NetRole::Host);
}

TEST_CASE("parseArgs: --host --port -> Host with custom port")
{
    const GameConfig cfg = parse({"--host", "--port", "1234"});
    REQUIRE(cfg.net.role == NetRole::Host);
    REQUIRE(cfg.net.port == 1234);
}

// ---- --dedicated ----

TEST_CASE("parseArgs: --dedicated -> Dedicated role")
{
    const GameConfig cfg = parse({"--dedicated"});
    REQUIRE(cfg.net.role == NetRole::Dedicated);
}

TEST_CASE("parseArgs: --dedicated --port -> Dedicated with custom port")
{
    const GameConfig cfg = parse({"--dedicated", "--port", "9000"});
    REQUIRE(cfg.net.role == NetRole::Dedicated);
    REQUIRE(cfg.net.port == 9000);
}

// ---- --connect ----

TEST_CASE("parseArgs: --connect bare host -> Client with default port")
{
    const GameConfig cfg = parse({"--connect", "192.168.1.42"});
    REQUIRE(cfg.net.role == NetRole::Client);
    REQUIRE(cfg.net.host == "192.168.1.42");
    REQUIRE(cfg.net.port == 7777);
}

TEST_CASE("parseArgs: --connect host:port -> Client with host and port")
{
    const GameConfig cfg = parse({"--connect", "game.example.com:5555"});
    REQUIRE(cfg.net.role == NetRole::Client);
    REQUIRE(cfg.net.host == "game.example.com");
    REQUIRE(cfg.net.port == 5555);
}

TEST_CASE("parseArgs: --connect uses rfind for colon (IPv4 with port)")
{
    const GameConfig cfg = parse({"--connect", "10.0.0.1:8080"});
    REQUIRE(cfg.net.host == "10.0.0.1");
    REQUIRE(cfg.net.port == 8080);
}

// ---- edge cases ----

TEST_CASE("parseArgs: --connect missing value -> Solo (arg consumed safely)")
{
    // --connect at the end with no following arg: condition `i+1 < argc` fails,
    // so the flag is treated as unknown and role stays Solo.
    const GameConfig cfg = parse({"--connect"});
    REQUIRE(cfg.net.role == NetRole::Solo);
}

TEST_CASE("parseArgs: --port missing value -> default port unchanged")
{
    const GameConfig cfg = parse({"--port"});
    REQUIRE(cfg.net.port == 7777);
}

TEST_CASE("parseArgs: unknown flag -> Solo role unchanged")
{
    const GameConfig cfg = parse({"--unknown-flag"});
    REQUIRE(cfg.net.role == NetRole::Solo);
}

TEST_CASE("parseArgs: --port before --host -> port applied")
{
    const GameConfig cfg = parse({"--port", "4000", "--host"});
    REQUIRE(cfg.net.role == NetRole::Host);
    REQUIRE(cfg.net.port == 4000);
}
