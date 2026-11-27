#include "Mu/Online/OnlineClient.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace mu::online {

using nexum::time::Seconds;

namespace {

// The room of the events that wait for the next tick. The first entry brings 15 to 20 messages (section 6.12
// of the protocol spec), and a list of NPCs gives one event for each; past the room, the queue grows instead
// of dropping an event.
constexpr std::size_t kEventRoom = 1024;

} // namespace

OnlineClient::OnlineClient(ServerAdapter& server, nexum::debug::Logger& log, std::optional<Seconds> stay)
    : m_server(&server), m_events(kEventRoom), m_view(log), m_stay(stay) {}

void OnlineClient::Start(Seconds now) {
    m_server->Start(now);
}

void OnlineClient::TakeEvents(Seconds now) {
    m_server->TakeEvents(now, m_events);
    if (!m_enteredWorld && m_server->Status().state == ServerState::InWorld) {
        m_enteredWorld = now;
    }
}

void OnlineClient::AdvanceClock(Seconds now) {
    m_server->AdvanceClock(now);
}

void OnlineClient::ApplyEvents(Seconds now) {
    const std::size_t applied =
        m_events.Drain([this](const world::WorldEvent& event) { m_view.Apply(event); });
    if (applied > 0) {
        m_server->EventsApplied(m_view, applied, now);
    }
}

OnlineStatus OnlineClient::Status(Seconds now) const noexcept {
    switch (m_server->Status().state) {
    case ServerState::Failed:
        return OnlineStatus::Failed;
    case ServerState::Ended:
        return OnlineStatus::ServerEnded;
    case ServerState::Connecting:
    case ServerState::InWorld:
        break;
    }
    if (m_stay && m_enteredWorld && now - *m_enteredWorld >= *m_stay) {
        return OnlineStatus::StayedLongEnough;
    }
    return OnlineStatus::Running;
}

std::string OnlineClient::Summary(Seconds now) const {
    const std::string server = m_server->Summary(now);
    if (!m_enteredWorld) {
        return "the session ends: never in the world, " + server;
    }
    const double inWorld = std::max(0.0, (now - *m_enteredWorld).count());
    return "the session ends: " + std::to_string(static_cast<std::uint64_t>(inWorld)) + " s in the world, " +
           server + ", objects in view: " + std::to_string(m_view.Objects().size());
}

} // namespace mu::online
