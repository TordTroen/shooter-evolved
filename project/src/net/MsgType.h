#pragma once
#include <cstdint>

// First byte of every network message. Defines the payload layout.
enum class MsgType : uint8_t
{
    // Reliable (server → client)
    AssignPlayerId = 1,  // server assigns the client its NetworkId
    StartGame      = 2,  // server tells clients to enter PlayingState

    // Unreliable (client → server)
    InputFrameMsg  = 10, // InputFrame (see InputFrame.h)
    FireIntentMsg  = 11, // FireIntent  (see FireIntent.h)

    // Unreliable (server → clients)
    Snapshot       = 20, // full state snapshot (see SnapshotMessage)
};
