#include "Mu/Session/MessageQueue.hpp"

namespace mu::session {

MessageQueue::MessageQueue(std::size_t reservedBytes, std::size_t reservedMessages) {
    m_bytes.reserve(reservedBytes);
    m_records.reserve(reservedMessages);
}

bool MessageQueue::Push(SessionEpoch epoch, std::span<const std::byte> message) {
    const bool fits =
        m_bytes.size() + message.size() <= m_bytes.capacity() && m_records.size() < m_records.capacity();
    m_records.push_back({epoch, m_bytes.size(), message.size()});
    m_bytes.insert(m_bytes.end(), message.begin(), message.end());
    return fits;
}

void MessageQueue::Clear() noexcept {
    m_bytes.clear();
    m_records.clear();
}

} // namespace mu::session
