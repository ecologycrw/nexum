#include "Mu/Online/OnlineClient.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

namespace mu::online {

using nexum::time::Seconds;

namespace {

constexpr nexum::debug::LogCategory kOnline{"online"};

// The room of the events that wait for the next tick. The first entry brings 15 to 20 messages (section 6.12
// of the protocol spec), and a list of NPCs gives one event for each; past the room, the queue grows instead
// of dropping an event.
constexpr std::size_t kEventRoom = 1024;

} // namespace

OnlineClient::OnlineClient(session::SessionOptions options, Transport& transport, nexum::debug::Logger& log,
                           std::optional<Seconds> stay)
    : m_transport(&transport), m_log(&log), m_session(std::move(options), transport, log),
      m_events(kEventRoom), m_view(log), m_stay(stay) {}

void OnlineClient::Start(Seconds now) {
    m_started = true;
    m_session.Start(now);
}

void OnlineClient::TakeNetwork(Seconds now) {
    m_transport->Deliver(m_session, now);
    if (m_session.WaitsForMap()) {
        EnterWorld(now);
    }
    if (m_translator) {
        m_session.TakeWorldMessages(
            [this](session::SessionEpoch /*epoch*/, std::span<const std::byte> message) {
                m_translator->Translate(message, m_events);
            });
    }
}

// There is no map to load yet, so the map is ready as soon as the data of the character came (TD-0033, M3).
void OnlineClient::EnterWorld(Seconds now) {
    if (auto finished = m_session.FinishLoading(); !finished) {
        m_log->Error(kOnline, finished.error().message);
        return;
    }
    m_translator.emplace(world::ObjectId{m_session.PlayerId()}, *m_log);
    m_enteredWorld = now;
}

void OnlineClient::AdvanceClock(Seconds now) {
    m_session.AdvanceClock(now);
}

void OnlineClient::ApplyEvents() {
    m_events.Drain([this](const world::WorldEvent& event) { m_view.Apply(event); });
}

OnlineStatus OnlineClient::Status(Seconds now) const noexcept {
    if (m_started && m_session.State() == session::SessionState::Disconnected) {
        return OnlineStatus::Failed;
    }
    if (m_stay && m_enteredWorld && now - *m_enteredWorld >= *m_stay) {
        return OnlineStatus::StayedLongEnough;
    }
    return OnlineStatus::Running;
}

std::string OnlineClient::Summary(Seconds now) const {
    const std::string pings = "pings sent: " + std::to_string(m_session.PingsSent());
    if (!m_enteredWorld) {
        return "the session ends: never in the world, " + pings;
    }
    const double inWorld = std::max(0.0, (now - *m_enteredWorld).count());
    return "the session ends: " + std::to_string(static_cast<std::uint64_t>(inWorld)) + " s in the world, " +
           pings + ", objects in view: " + std::to_string(m_view.Objects().size());
}

} // namespace mu::online
