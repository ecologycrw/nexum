#pragma once

// The transport of the real network: a nexum::net::Connection for each endpoint the session opens, each with
// a ClientCodec of its own (Mu/Protocol/ClientCodec.hpp), so both counters of the cipher start at 0 with each
// connection (section 4.1 of the protocol spec of the knowledge center). Only one connection is open at a
// time.

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Mu/Online/Transport.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Net/Connection.hpp"

namespace mu::online {

class NetworkTransport final : public Transport {
public:
    NetworkTransport();

    void Open(const session::Endpoint& endpoint) override;
    [[nodiscard]] nexum::core::Result<void> Send(std::span<const std::byte> message) override;
    void Close() override;
    std::size_t Deliver(session::Session& session, nexum::time::Seconds now) override;

private:
    // A connection that could not even start: the session hears it as the end of that connection.
    struct FailedStart {
        session::SessionEpoch epoch;
        std::string reason;
    };

    std::unique_ptr<nexum::net::Connection> m_open;
    session::SessionEpoch m_openEpoch;
    std::optional<FailedStart> m_failedStart;
    // The connections closed since the last Deliver. Each waits for its thread to end when it is destroyed,
    // which Deliver does after handing over the events, so a connection never dies while it is being read.
    std::vector<std::unique_ptr<nexum::net::Connection>> m_closed;
};

} // namespace mu::online
