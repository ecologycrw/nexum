#pragma once

// The client on its way to the world and in it, with the server behind its contract
// (Mu/Online/ServerAdapter.hpp, ADR 0035): the queue of the events, the view of the world (Mu/World) and the
// time asked in the world, each in its step of the frame (docs/ARCHITECTURE.md, "O quadro"). Step 2 takes the
// events of the server, without applying them; step 3 runs the wall clocks of the server; each tick applies
// the events, in the order they came, before the systems, and tells the server. The client does not know
// where the events come from, and it never reads a clock: each call brings the monotonic time of the frame.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "Mu/Online/ServerAdapter.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"
#include "Nexum/Time/Seconds.hpp"

namespace mu::online {

enum class OnlineStatus : std::uint8_t {
    Running,
    // In the world for the time that was asked.
    StayedLongEnough,
    // The server ended without a failure, as a replay that reached its end.
    ServerEnded,
    // The server ended with an error, which the log already has.
    Failed,
};

class OnlineClient {
public:
    // `server` and `log` must outlive the client. `stay` is how long to stay in the world before the status
    // says so; empty, the client stays until it is closed.
    OnlineClient(ServerAdapter& server, nexum::debug::Logger& log, std::optional<nexum::time::Seconds> stay);

    // Starts the server.
    void Start(nexum::time::Seconds now);

    // Step 2 of the frame: takes the events of the server for the next tick, and notes the entry in the
    // world.
    void TakeEvents(nexum::time::Seconds now);

    // Step 3 of the frame: the wall clocks of the server.
    void AdvanceClock(nexum::time::Seconds now);

    // The start of each tick, before the systems: applies the events, in the order they came (INV-01), and,
    // when there were any, tells the server how many and how the view ended up. An event of an epoch older
    // than the one of the server is dropped and counted instead, and the log says so once in the tick. `now`
    // is the time of the frame.
    void ApplyEvents(nexum::time::Seconds now);

    [[nodiscard]] OnlineStatus Status(nexum::time::Seconds now) const noexcept;

    // One line about the session: the time in the world, the words of the server and the objects in view.
    [[nodiscard]] std::string Summary(nexum::time::Seconds now) const;

    [[nodiscard]] const world::WorldView& View() const noexcept { return m_view; }
    // The events taken and not yet applied.
    [[nodiscard]] std::size_t PendingEvents() const noexcept { return m_events.Count(); }
    // The events that the ticks dropped for an older epoch, since the start.
    [[nodiscard]] std::uint64_t DroppedEvents() const noexcept { return m_droppedEvents; }

private:
    ServerAdapter* m_server;
    nexum::debug::Logger* m_log;
    world::EventQueue m_events;
    world::WorldView m_view;
    std::optional<nexum::time::Seconds> m_stay;
    std::optional<nexum::time::Seconds> m_enteredWorld;
    std::uint64_t m_droppedEvents = 0;
};

} // namespace mu::online
