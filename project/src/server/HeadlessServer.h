#pragma once

#include "../core/GameConfig.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

class Net;

// Graphics-free server loop for the `fps_server` process. Mirrors Game::run()'s
// network pumping, but with no window/event pump and console status output in
// place of the old DedicatedServerState ImGui panel.
class HeadlessServer
{
public:
    explicit HeadlessServer(const NetConfig& cfg);
    ~HeadlessServer();

    HeadlessServer(const HeadlessServer&)            = delete;
    HeadlessServer& operator=(const HeadlessServer&) = delete;

    // Runs until Ctrl+C (SIGINT/SIGTERM) requests shutdown.
    void run();

private:
    std::unique_ptr<Net> m_net;

    // Set by the SIGINT/SIGTERM handler. Static because OS signal handlers cannot
    // carry instance context; only one HeadlessServer runs per process.
    static std::atomic<bool> s_shouldStop;
    static void handle_stop_signal(int signal);

    void print_status(uint32_t serverTick) const;
};
