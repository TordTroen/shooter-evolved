#pragma once

enum class NetRole
{
    Solo,       // no networking
    Host,       // listen-server: runs Server + local Client via MockTransport
    Client,     // connects to a remote host
    Dedicated,  // server-only process (no local client)
};
