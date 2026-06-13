#include "Net.h"
#include "Server.h"
#include "Client.h"
#include "GnsTransport.h"

#include <iostream>

static constexpr uint16_t kDefaultPort = 7777;

Net::Net(NetRole role, const std::string& host, uint16_t port)
    : m_role(role)
{
    if (port == 0) port = kDefaultPort;

    switch (role)
    {
        case NetRole::Solo:
            std::cout << "[Net] Role: Solo (no networking)\n";
            break;

        case NetRole::Host:
        {
            std::cout << "[Net] Role: Host — server on port " << port
                      << ", local client connecting to 127.0.0.1:" << port << "\n";
            // Server listens via GNS. Local client also connects via GNS to localhost
            // so both sides share identical code paths (no special listen-server shortcuts).
            auto serverTransport = std::make_unique<GnsTransport>();
            m_server = std::make_unique<Server>(std::move(serverTransport));
            m_server->start(port);

            auto clientTransport = std::make_unique<GnsTransport>();
            m_client = std::make_unique<Client>(std::move(clientTransport));
            m_client->connect("127.0.0.1", port);
            break;
        }

        case NetRole::Client:
        {
            std::cout << "[Net] Role: Client — connecting to " << host << ":" << port << "\n";
            auto clientTransport = std::make_unique<GnsTransport>();
            m_client = std::make_unique<Client>(std::move(clientTransport));
            m_client->connect(host.c_str(), port);
            break;
        }

        case NetRole::Dedicated:
        {
            std::cout << "[Net] Role: Dedicated — server on port " << port << "\n";
            auto serverTransport = std::make_unique<GnsTransport>();
            m_server = std::make_unique<Server>(std::move(serverTransport));
            m_server->start(port);
            break;
        }
    }
}

Net::~Net() = default;

void Net::pump(float dt)
{
    if (m_server) m_server->tick(dt);
    if (m_client) m_client->pump();
}
