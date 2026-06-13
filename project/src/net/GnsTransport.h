#pragma once

#include "Transport.h"
#include <memory>

// GameNetworkingSockets-backed transport. GNS types are confined to the .cpp;
// nothing from the GNS headers leaks through this header.
class GnsTransport : public Transport
{
public:
    GnsTransport();
    ~GnsTransport() override;

    GnsTransport(const GnsTransport&)            = delete;
    GnsTransport& operator=(const GnsTransport&) = delete;

    void startServer(uint16_t port) override;
    ConnectionId connectTo(const char* host, uint16_t port) override;
    void disconnect(ConnectionId conn) override;
    void closeServer() override;

    void send(ConnectionId conn, Channel ch, const std::byte* data, size_t len) override;
    void poll(std::vector<IncomingMessage>& out) override;

    // Called by the static GNS callback; not for external use.
    void handleStatusChanged(void* pInfo);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
