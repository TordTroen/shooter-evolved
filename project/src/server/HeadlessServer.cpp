#include "HeadlessServer.h"
#include "../net/Net.h"
#include "../net/Server.h"

#include <chrono>
#include <csignal>
#include <iostream>
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

    std::cout << "[HeadlessServer] Running. Press Ctrl+C to quit.\n";

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

    std::cout << "[HeadlessServer] Shutdown requested, exiting.\n";
}

void HeadlessServer::print_status(uint32_t serverTick) const
{
    Server* const server = m_net->server();
    const NetworkId leader = server->leaderNetId();

    std::cout << "[HeadlessServer] tick=" << serverTick
              << " players=" << server->playerCount()
              << " leader=";
    if (leader)
    {
        std::cout << leader.value;
    }
    else
    {
        std::cout << "none";
    }
    std::cout << "\n";
}
