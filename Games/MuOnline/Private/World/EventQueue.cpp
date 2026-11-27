#include "Mu/World/EventQueue.hpp"

namespace mu::world {

EventQueue::EventQueue(std::size_t reservedEvents) {
    m_events.reserve(reservedEvents);
}

bool EventQueue::Push(const WorldEvent& event) {
    const bool fits = m_events.size() < m_events.capacity();
    m_events.push_back(event);
    return fits;
}

} // namespace mu::world
