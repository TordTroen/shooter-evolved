#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

using ConnectionId = uint32_t;
static constexpr ConnectionId kInvalidConnection = 0;

enum class Channel { Reliable, Unreliable };

struct IncomingMessage
{
    ConnectionId         from;
    Channel              channel;
    std::vector<std::byte> data;
};

// Abstract transport layer. GNS-specific types do not leak past GnsTransport.cpp.
class Transport
{
public:
    virtual ~Transport() = default;

    virtual void startServer(uint16_t port)                              = 0;
    virtual ConnectionId connectTo(const char* host, uint16_t port)     = 0;
    virtual void disconnect(ConnectionId conn)                           = 0;
    virtual void closeServer()                                           = 0;

    virtual void send(ConnectionId conn, Channel ch,
                      const std::byte* data, size_t len)                = 0;
    virtual void poll(std::vector<IncomingMessage>& out)                = 0;

    std::function<void(ConnectionId)> onConnect;
    std::function<void(ConnectionId)> onDisconnect;
};
