#include "GnsTransport.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include <cstring>
#include <iostream>
#include <unordered_map>

// ---------------------------------------------------------------------------
// GNS lifetime - reference-counted per process.
// ---------------------------------------------------------------------------
namespace
{
    int  s_refCount = 0;

    void gnsAddRef()
    {
        if (s_refCount++ > 0) return;
        SteamNetworkingErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg))
            std::cerr << "[GNS] Init failed: " << errMsg << "\n";
        else
            SteamNetworkingUtils()->SetGlobalConfigValueInt32(
                k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);
    }

    void gnsRelease()
    {
        if (--s_refCount <= 0) { s_refCount = 0; GameNetworkingSockets_Kill(); }
    }
}

// ---------------------------------------------------------------------------
// Global routing maps - route GNS callbacks to the owning GnsTransport.
//   listenSock → transport  (server side, for newly accepted connections)
//   conn       → transport  (client connections and accepted server connections)
// ---------------------------------------------------------------------------
static std::unordered_map<uint32_t, GnsTransport*> s_listenMap;
static std::unordered_map<uint32_t, GnsTransport*> s_connMap;

static void gnsConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
    // Try to look up by connection first (handles client connections and already-
    // accepted server connections).
    auto it = s_connMap.find(static_cast<uint32_t>(pInfo->m_hConn));
    if (it != s_connMap.end()) { it->second->handleStatusChanged(pInfo); return; }

    // New inbound connection: look up by listen socket.
    auto jt = s_listenMap.find(static_cast<uint32_t>(pInfo->m_info.m_hListenSocket));
    if (jt != s_listenMap.end()) { jt->second->handleStatusChanged(pInfo); }
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct GnsTransport::Impl
{
    ISteamNetworkingSockets* sockets    = nullptr;
    HSteamListenSocket       listenSock = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup       pollGroup  = k_HSteamNetPollGroup_Invalid;
    HSteamNetConnection      clientConn = k_HSteamNetConnection_Invalid;
    bool                     isServer   = false;
};

// ---------------------------------------------------------------------------
GnsTransport::GnsTransport()
    : m_impl(std::make_unique<Impl>())
{
    gnsAddRef();
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(
        gnsConnectionStatusChanged);
    m_impl->sockets = SteamNetworkingSockets();
}

GnsTransport::~GnsTransport()
{
    closeServer();
    if (m_impl->clientConn != k_HSteamNetConnection_Invalid)
    {
        s_connMap.erase(static_cast<uint32_t>(m_impl->clientConn));
        m_impl->sockets->CloseConnection(m_impl->clientConn, 0, nullptr, false);
    }
    gnsRelease();
}

void GnsTransport::startServer(uint16_t port)
{
    m_impl->isServer = true;
    m_impl->pollGroup = m_impl->sockets->CreatePollGroup();

    SteamNetworkingIPAddr addr;
    addr.Clear();
    addr.m_port = port;

    m_impl->listenSock = m_impl->sockets->CreateListenSocketIP(addr, 0, nullptr);
    if (m_impl->listenSock == k_HSteamListenSocket_Invalid)
        std::cerr << "[GNS] Failed to create listen socket on port " << port << "\n";
    else
    {
        s_listenMap[static_cast<uint32_t>(m_impl->listenSock)] = this;
        std::cout << "[GNS] Server listening on port " << port << "\n";
    }
}

ConnectionId GnsTransport::connectTo(const char* host, uint16_t port)
{
    SteamNetworkingIPAddr addr;
    addr.Clear();
    addr.ParseString(host);
    addr.m_port = port;

    char addrStr[SteamNetworkingIPAddr::k_cchMaxString];
    addr.ToString(addrStr, sizeof(addrStr), true);
    std::cout << "[GNS] Connecting to " << addrStr << "\n";

    m_impl->clientConn = m_impl->sockets->ConnectByIPAddress(addr, 0, nullptr);
    if (m_impl->clientConn == k_HSteamNetConnection_Invalid)
    {
        std::cerr << "[GNS] ConnectByIPAddress failed for " << addrStr << "\n";
        return kInvalidConnection;
    }
    std::cout << "[GNS] Connection handle " << m_impl->clientConn << " created\n";
    s_connMap[static_cast<uint32_t>(m_impl->clientConn)] = this;
    return static_cast<ConnectionId>(m_impl->clientConn);
}

void GnsTransport::disconnect(ConnectionId conn)
{
    s_connMap.erase(conn);
    m_impl->sockets->CloseConnection(
        static_cast<HSteamNetConnection>(conn), 0, "disconnect", true);
}

void GnsTransport::closeServer()
{
    if (m_impl->listenSock != k_HSteamListenSocket_Invalid)
    {
        s_listenMap.erase(static_cast<uint32_t>(m_impl->listenSock));
        m_impl->sockets->CloseListenSocket(m_impl->listenSock);
        m_impl->listenSock = k_HSteamListenSocket_Invalid;
    }
    if (m_impl->pollGroup != k_HSteamNetPollGroup_Invalid)
    {
        m_impl->sockets->DestroyPollGroup(m_impl->pollGroup);
        m_impl->pollGroup = k_HSteamNetPollGroup_Invalid;
    }
}

void GnsTransport::send(ConnectionId conn, Channel ch, const std::byte* data, size_t len)
{
    const int flags = (ch == Channel::Reliable)
        ? k_nSteamNetworkingSend_Reliable
        : k_nSteamNetworkingSend_Unreliable;
    m_impl->sockets->SendMessageToConnection(
        static_cast<HSteamNetConnection>(conn),
        data, static_cast<uint32_t>(len), flags, nullptr);
}

void GnsTransport::poll(std::vector<IncomingMessage>& out)
{
    m_impl->sockets->RunCallbacks();

    constexpr int kMaxMsgs = 64;
    ISteamNetworkingMessage* pMessages[kMaxMsgs];
    int numMsgs = 0;

    if (m_impl->isServer && m_impl->pollGroup != k_HSteamNetPollGroup_Invalid)
        numMsgs = m_impl->sockets->ReceiveMessagesOnPollGroup(
            m_impl->pollGroup, pMessages, kMaxMsgs);
    else if (m_impl->clientConn != k_HSteamNetConnection_Invalid)
        numMsgs = m_impl->sockets->ReceiveMessagesOnConnection(
            m_impl->clientConn, pMessages, kMaxMsgs);

    for (int i = 0; i < numMsgs; ++i)
    {
        ISteamNetworkingMessage* msg = pMessages[i];
        IncomingMessage im;
        im.from    = static_cast<ConnectionId>(msg->m_conn);
        im.channel = (msg->m_nFlags & k_nSteamNetworkingSend_Reliable)
                       ? Channel::Reliable : Channel::Unreliable;
        const auto* bytes = static_cast<const std::byte*>(msg->m_pData);
        im.data.assign(bytes, bytes + msg->m_cbSize);
        out.push_back(std::move(im));
        msg->Release();
    }
}

void GnsTransport::handleStatusChanged(void* pInfoVoid)
{
    auto* pInfo = static_cast<SteamNetConnectionStatusChangedCallback_t*>(pInfoVoid);
    const auto newState = pInfo->m_info.m_eState;
    const auto conn     = pInfo->m_hConn;

    switch (newState)
    {
        case k_ESteamNetworkingConnectionState_Connecting:
            if (m_impl->isServer)
            {
                m_impl->sockets->AcceptConnection(conn);
                m_impl->sockets->SetConnectionPollGroup(conn, m_impl->pollGroup);
                s_connMap[static_cast<uint32_t>(conn)] = this;
                std::cout << "[GNS] Accepted connection " << conn << "\n";
                if (onConnect) onConnect(static_cast<ConnectionId>(conn));
            }
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            if (!m_impl->isServer)
            {
                std::cout << "[GNS] Connected to server\n";
                if (onConnect) onConnect(static_cast<ConnectionId>(conn));
            }
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            const char* reason = (newState == k_ESteamNetworkingConnectionState_ClosedByPeer)
                ? "closed by peer" : "problem detected locally";
            std::cout << "[GNS] Connection " << conn << " lost (" << reason
                      << "): " << pInfo->m_info.m_szEndDebug << "\n";
            if (onDisconnect) onDisconnect(static_cast<ConnectionId>(conn));
            s_connMap.erase(static_cast<uint32_t>(conn));
            m_impl->sockets->CloseConnection(conn, 0, nullptr, false);
            break;
        }

        default: break;
    }
}
