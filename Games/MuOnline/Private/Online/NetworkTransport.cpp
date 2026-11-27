#include "Mu/Online/NetworkTransport.hpp"

#include <utility>

#include "Mu/Protocol/ClientCodec.hpp"

namespace mu::online {

namespace {

// A session opens at most two connections in a row (the Connect Server, then the Game Server) before a
// Deliver destroys the closed ones.
constexpr std::size_t kClosedRoom = 2;

} // namespace

NetworkTransport::NetworkTransport() {
    m_closed.reserve(kClosedRoom);
}

void NetworkTransport::Open(const session::Endpoint& endpoint) {
    Close();
    m_openEpoch = endpoint.epoch;
    auto connection =
        nexum::net::Connection::Open({.host = endpoint.host, .port = endpoint.port},
                                     std::make_unique<protocol::ClientCodec>(endpoint.encryption));
    if (!connection) {
        m_failedStart =
            FailedStart{.epoch = endpoint.epoch,
                        .reason = "the connection could not start: " + connection.error().message};
        return;
    }
    m_open = std::move(*connection);
}

nexum::core::Result<void> NetworkTransport::Send(std::span<const std::byte> message) {
    if (!m_open) {
        return nexum::core::MakeError("no connection is open");
    }
    return m_open->Send(message);
}

void NetworkTransport::Close() {
    if (m_open) {
        m_open->Close();
        m_closed.push_back(std::move(m_open));
    }
}

std::size_t NetworkTransport::Deliver(session::Session& session, nexum::time::Seconds now) {
    std::size_t count = 0;
    if (m_failedStart) {
        const FailedStart failed = std::move(*m_failedStart);
        m_failedStart.reset();
        session.Handle(failed.epoch,
                       {.kind = nexum::net::EventKind::Closed, .message = {}, .reason = failed.reason}, now);
        ++count;
    }
    if (m_open) {
        // The session may close this connection and open the next while it handles an event; the events left
        // keep the epoch of this one, so the session drops them.
        nexum::net::Connection& reading = *m_open;
        const session::SessionEpoch epoch = m_openEpoch;
        count += reading.Poll([&](const nexum::net::Event& event) { session.Handle(epoch, event, now); });
    }
    m_closed.clear();
    return count;
}

} // namespace mu::online
