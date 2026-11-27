#pragma once

// What the online client needs of the network: the link of the session (Mu/Session/Session.hpp), and a way to
// hand the session the events that arrived. The real network is NetworkTransport; the tests bring their own,
// with the messages in memory.

#include <cstddef>

#include "Mu/Session/Session.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::online {

class Transport : public session::SessionLink {
public:
    // Gives each event that arrived since the last call to `session.Handle`, in order, with the epoch of its
    // connection, and gives how many. It never waits for the network (INV-06). The session may open and close
    // connections while it handles an event: the connection being read stays alive until the call ends.
    virtual std::size_t Deliver(session::Session& session, nexum::time::Seconds now) = 0;
};

} // namespace mu::online
