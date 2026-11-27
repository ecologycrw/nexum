#pragma once

// The events of the world that wait for the tick: the frame takes the messages and translates them, and the
// next tick applies the events, in the order they came (docs/ARCHITECTURE.md, "O quadro"). Each event keeps
// the epoch of its message, so the tick can drop the events of a session that is over (D-10 of the reference
// spec, item 5); the view of the world never sees the epoch. The room is reserved once, so holding events
// never asks for memory; a queue that passes the room grows instead of dropping anything, because the order
// matters (IV.14.6 of the reference spec).

#include <cstddef>
#include <vector>

#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/World/WorldEvent.hpp"

namespace mu::world {

class EventQueue {
public:
    explicit EventQueue(std::size_t reservedEvents);

    // Puts the event at the end, with the epoch of the message it came from. False means it did not fit in
    // the reserved room: the queue grew, and kept the event.
    bool Push(session::SessionEpoch epoch, const WorldEvent& event);

    // Gives every event to `handler(session::SessionEpoch, const WorldEvent&)`, in order, then empties the
    // queue and keeps its room. Gives how many.
    template <typename Handler>
    std::size_t Drain(Handler&& handler) {
        for (const Queued& queued : m_events) {
            handler(queued.epoch, queued.event);
        }
        const std::size_t count = m_events.size();
        m_events.clear();
        return count;
    }

    void Clear() noexcept { m_events.clear(); }

    [[nodiscard]] std::size_t Count() const noexcept { return m_events.size(); }
    [[nodiscard]] bool Empty() const noexcept { return m_events.empty(); }

private:
    struct Queued {
        session::SessionEpoch epoch;
        WorldEvent event;
    };

    std::vector<Queued> m_events;
};

} // namespace mu::world
