#include "Mu/Online/LiveServer.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <utility>

namespace mu::online {

using nexum::time::Seconds;

namespace {

constexpr nexum::debug::LogCategory kOnline{"online"};

} // namespace

LiveServer::LiveServer(session::SessionOptions options, Transport& transport, nexum::debug::Logger& log)
    : m_transport(&transport), m_log(&log), m_session(std::move(options), transport, log) {}

void LiveServer::Start(Seconds now) {
    m_started = true;
    m_session.Start(now);
}

void LiveServer::TakeEvents(Seconds now, world::EventQueue& events) {
    m_transport->Deliver(m_session, now);
    if (m_session.WaitsForMap()) {
        EnterWorld();
    }
    if (m_translator) {
        m_session.TakeWorldMessages(
            [this, &events](session::SessionEpoch epoch, std::span<const std::byte> message) {
                m_translator->Translate(epoch, message, events);
            });
    }
}

// There is no map to load yet, so the map is ready as soon as the data of the character came (TD-0033, M3).
void LiveServer::EnterWorld() {
    if (auto finished = m_session.FinishLoading(); !finished) {
        m_log->Error(kOnline, finished.error().message);
        return;
    }
    m_translator.emplace(world::ObjectId{m_session.PlayerId()}, *m_log);
}

void LiveServer::AdvanceClock(Seconds now) {
    m_session.AdvanceClock(now);
}

void LiveServer::EventsApplied(const world::WorldView& /*view*/, std::size_t /*count*/, Seconds /*now*/) {}

ServerStatus LiveServer::Status() const noexcept {
    if (m_started && m_session.State() == session::SessionState::Disconnected) {
        return ServerStatus{.state = ServerState::Failed, .reason = m_session.Failure()};
    }
    if (m_session.State() == session::SessionState::InWorld) {
        return ServerStatus{.state = ServerState::InWorld, .reason = {}};
    }
    return ServerStatus{};
}

std::string LiveServer::Summary(Seconds /*now*/) const {
    return "pings sent: " + std::to_string(m_session.PingsSent());
}

} // namespace mu::online
