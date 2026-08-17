#include "HeadlessServer.h"
#include "../net/Net.h"
#include "../net/Server.h"

#include <chrono>
#include <csignal>
#include <print>
#include <thread>

std::atomic<bool> HeadlessServer::s_shouldStop{ false };

HeadlessServer::HeadlessServer(const NetConfig& cfg)
    : m_net(std::make_unique<Net>(cfg.role, cfg.host, cfg.port))
{
    std::signal(SIGINT, handle_stop_signal);
    std::signal(SIGTERM, handle_stop_signal);
}

HeadlessServer::~HeadlessServer() = default;

void HeadlessServer::handle_stop_signal(int /*signal*/)
{
    s_shouldStop = true;
}

void HeadlessServer::run()
{
    constexpr float kMinLoopHz = 60.0f;
    constexpr auto  kMinLoopPeriod =
        std::chrono::duration<float>(1.0f / kMinLoopHz);
    constexpr auto  kStatusPeriod = std::chrono::seconds(1);

    std::println("[HeadlessServer] Running. Press Ctrl+C to quit.");

    auto lastTime   = std::chrono::steady_clock::now();
    auto lastStatus = lastTime;

    while (!s_shouldStop)
    {
        const auto  now = std::chrono::steady_clock::now();
        const float dt  = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        m_net->pump(dt);

        if (now - lastStatus >= kStatusPeriod)
        {
            lastStatus = now;
            print_status(m_net->server()->currentTick());
        }

        const auto elapsed = std::chrono::steady_clock::now() - now;
        if (elapsed < kMinLoopPeriod)
        {
            std::this_thread::sleep_for(kMinLoopPeriod - elapsed);
        }
    }

    std::println("[HeadlessServer] Shutdown requested, exiting.");
}

void HeadlessServer::print_status(uint32_t serverTick) const
{
    Server* const server = m_net->server();
    const NetworkId leader = server->leaderNetId();

    if (leader)
    {
        std::println("[HeadlessServer] tick={} players={} leader={}",
            serverTick, server->playerCount(), leader.value);
    }
    else
    {
        std::println("[HeadlessServer] tick={} players={} leader=none",
            serverTick, server->playerCount());
    }
}
