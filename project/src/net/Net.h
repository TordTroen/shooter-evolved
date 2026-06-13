#pragma once

#include "NetRole.h"

#include <cstdint>
#include <memory>
#include <string>

class Server;
class Client;

class Net
{
public:
    Net(NetRole role, const std::string& host, uint16_t port);
    ~Net();

    Net(const Net&)            = delete;
    Net& operator=(const Net&) = delete;

    // Call once per game loop iteration, before the active state's update().
    void pump(float dt);

    NetRole role()   const { return m_role; }
    Server* server()       { return m_server.get(); }
    Client* client()       { return m_client.get(); }

private:
    NetRole                m_role;
    std::unique_ptr<Server> m_server;
    std::unique_ptr<Client> m_client;
};
