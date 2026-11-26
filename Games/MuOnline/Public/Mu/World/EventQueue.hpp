#pragma once

// The events of the world that wait for the tick: the frame takes the messages and translates them, and the
// next tick applies the events, in the order they came (docs/ARCHITECTURE.md, "O quadro"). The room is
// reserved once, so holding events never asks for memory; a queue that passes the room grows instead of
// dropping anything, because the order matters (IV.14.6 of the reference spec).

#include <cstddef>
#include <vector>

#include "Mu/World/WorldEvent.hpp"

namespace mu::world {

class EventQueue {
public:
    explicit EventQueue(std::size_t reservedEvents);

    // Puts the event at the end. False means it did not fit in the reserved room: the queue grew, and kept
    // the event.
    bool Push(const WorldEvent& event);

    // Gives every event to `handler(const WorldEvent&)`, in order, then empties the queue and keeps its room.
    // Gives how many.
    template <typename Handler>
    std::size_t Drain(Handler&& handler) {
        for (const WorldEvent& event : m_events) {
            handler(event);
        }
        const std::size_t count = m_events.size();
        m_events.clear();
        return count;
    }

    void Clear() noexcept { m_events.clear(); }

    [[nodiscard]] std::size_t Count() const noexcept { return m_events.size(); }
    [[nodiscard]] bool Empty() const noexcept { return m_events.empty(); }

private:
    std::vector<WorldEvent> m_events;
};

} // namespace mu::world
