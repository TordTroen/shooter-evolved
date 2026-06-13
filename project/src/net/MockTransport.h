#pragma once

#include "Transport.h"

#include <memory>
#include <mutex>
#include <queue>
#include <utility>

// Two paired MockTransports share a single Endpoints struct. Messages sent by
// side A are queued and read by side B, and vice versa. createPair() returns
// the two halves. Connections are established automatically on first use.
class MockTransport : public Transport
{
public:
    static std::pair<std::unique_ptr<MockTransport>, std::unique_ptr<MockTransport>>
    createPair();

    ~MockTransport() override = default;

    void startServer(uint16_t) override { /* in-process: no-op */ }
    ConnectionId connectTo(const char*, uint16_t) override;
    void disconnect(ConnectionId) override {}
    void closeServer() override {}

    void send(ConnectionId conn, Channel ch, const std::byte* data, size_t len) override;
    void poll(std::vector<IncomingMessage>& out) override;

private:
    struct Endpoints
    {
        std::queue<IncomingMessage> aToB;
        std::queue<IncomingMessage> bToA;
        std::mutex                  mutex;
        bool connected = false;
    };

    MockTransport(std::shared_ptr<Endpoints> shared, bool isA);

    std::shared_ptr<Endpoints> m_shared;
    bool                       m_isA;
    bool                       m_connectedFired = false;
};
