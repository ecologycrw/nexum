#pragma once

// The client on its way to the world and in it: the session (Mu/Session), the translation of its messages
// into neutral events (Mu/Translate) and the view of the world (Mu/World), each in its step of the frame
// (docs/ARCHITECTURE.md, "O quadro"). Step 2 takes the events of the network and translates the messages of
// the world, without applying them; step 3 runs the wall clocks of the session; each tick applies the events,
// in the order they came, before the systems. The client never reads a clock: each call brings the monotonic
// time of the frame.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "Mu/Online/Transport.hpp"
#include "Mu/Session/Session.hpp"
#include "Mu/Translate/Translator.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::online {

enum class OnlineStatus : std::uint8_t {
    Running,
    // In the world for the time that was asked.
    StayedLongEnough,
    // The session ended with an error, which the log already has.
    Failed,
};

class OnlineClient {
public:
    // `transport` and `log` must outlive the client. `stay` is how long to stay in the world before the
    // status says so; empty, the client stays until it is closed.
    OnlineClient(session::SessionOptions options, Transport& transport, nexum::debug::Logger& log,
                 std::optional<nexum::time::Seconds> stay);

    // Opens the Connect Server.
    void Start(nexum::time::Seconds now);

    // Step 2 of the frame: hands the events of the network to the session, enters the world when the data of
    // the character came, and translates the messages of the world into events for the next tick.
    void TakeNetwork(nexum::time::Seconds now);

    // Step 3 of the frame: the deadlines of the session and the ping.
    void AdvanceClock(nexum::time::Seconds now);

    // The start of each tick, before the systems: applies the events, in the order they came (INV-01).
    void ApplyEvents();

    [[nodiscard]] OnlineStatus Status(nexum::time::Seconds now) const noexcept;

    // One line about the session: the time in the world, the pings sent and the objects in view.
    [[nodiscard]] std::string Summary(nexum::time::Seconds now) const;

    [[nodiscard]] session::SessionState State() const noexcept { return m_session.State(); }
    [[nodiscard]] const world::WorldView& View() const noexcept { return m_view; }
    // The events translated and not yet applied.
    [[nodiscard]] std::size_t PendingEvents() const noexcept { return m_events.Count(); }

private:
    void EnterWorld(nexum::time::Seconds now);

    Transport* m_transport;
    nexum::debug::Logger* m_log;
    session::Session m_session;
    // Born with the id of the hero, which the Game Server gives (section 6.5 of the protocol spec).
    std::optional<translate::Translator> m_translator;
    world::EventQueue m_events;
    world::WorldView m_view;
    std::optional<nexum::time::Seconds> m_stay;
    std::optional<nexum::time::Seconds> m_enteredWorld;
    bool m_started = false;
};

} // namespace mu::online
