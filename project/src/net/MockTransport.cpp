#include "MockTransport.h"

#include <cstring>

MockTransport::MockTransport(std::shared_ptr<Endpoints> shared, bool isA)
    : m_shared(std::move(shared))
    , m_isA(isA)
{
}

std::pair<std::unique_ptr<MockTransport>, std::unique_ptr<MockTransport>>
MockTransport::createPair()
{
    auto shared = std::make_shared<Endpoints>();
    auto a = std::unique_ptr<MockTransport>(new MockTransport(shared, true));
    auto b = std::unique_ptr<MockTransport>(new MockTransport(shared, false));
    return { std::move(a), std::move(b) };
}

ConnectionId MockTransport::connectTo(const char*, uint16_t)
{
    // Side A calls connectTo; side B's first poll sees the connection.
    std::lock_guard lock(m_shared->mutex);
    m_shared->connected = true;
    return 1; // single virtual connection
}

void MockTransport::send(ConnectionId, Channel ch, const std::byte* data, size_t len)
{
    IncomingMessage msg;
    msg.from    = 1;
    msg.channel = ch;
    msg.data.assign(data, data + len);

    std::lock_guard lock(m_shared->mutex);
    if (m_isA)
        m_shared->aToB.push(std::move(msg));
    else
        m_shared->bToA.push(std::move(msg));
}

void MockTransport::poll(std::vector<IncomingMessage>& out)
{
    std::unique_lock lock(m_shared->mutex);

    // Notify once when the other side connected.
    if (!m_connectedFired && m_shared->connected)
    {
        m_connectedFired = true;
        lock.unlock();
        if (onConnect) onConnect(1);
        lock.lock();
    }

    auto& inboundQueue = m_isA ? m_shared->bToA : m_shared->aToB;
    while (!inboundQueue.empty())
    {
        out.push_back(std::move(inboundQueue.front()));
        inboundQueue.pop();
    }
}
