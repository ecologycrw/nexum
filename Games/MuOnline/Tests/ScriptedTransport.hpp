#pragma once

// A transport with the events in memory, for the tests of the online client and of the recorder: the test
// queues them, each with the epoch of the connection open when it was queued, and the next Deliver hands them
// over. It keeps each message the session sent.

#include <cstddef>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Mu/Online/Transport.hpp"
#include "Mu/Session/Session.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Net/Connection.hpp"
#include "Nexum/Time/Seconds.hpp"
#include "SessionMessages.hpp"

namespace test_transport {

class ScriptedTransport final : public mu::online::Transport {
public:
    void Open(const mu::session::Endpoint& endpoint) override {
        opened.push_back(endpoint);
        m_epoch = endpoint.epoch;
    }

    nexum::core::Result<void> Send(std::span<const std::byte> message) override {
        sent.emplace_back(message.begin(), message.end());
        return {};
    }

    void Close() override { ++closes; }

    std::size_t Deliver(mu::session::Session& session, nexum::time::Seconds now) override {
        std::vector<Queued> queued = std::exchange(m_queued, {});
        for (const Queued& event : queued) {
            session.Handle(event.epoch,
                           {.kind = event.kind, .message = event.message, .reason = event.reason}, now);
        }
        ++deliveries;
        return queued.size();
    }

    void Connected() { m_queued.push_back({m_epoch, nexum::net::EventKind::Connected, {}, {}}); }
    void Message(session_messages::Bytes message) {
        m_queued.push_back({m_epoch, nexum::net::EventKind::Message, std::move(message), {}});
    }
    void Closed(std::string reason) {
        m_queued.push_back({m_epoch, nexum::net::EventKind::Closed, {}, std::move(reason)});
    }

    std::vector<mu::session::Endpoint> opened;
    std::vector<session_messages::Bytes> sent;
    int closes = 0;
    int deliveries = 0;

private:
    struct Queued {
        mu::session::SessionEpoch epoch;
        nexum::net::EventKind kind = nexum::net::EventKind::Closed;
        session_messages::Bytes message;
        std::string reason;
    };

    std::vector<Queued> m_queued;
    mu::session::SessionEpoch m_epoch;
};

} // namespace test_transport
