#pragma once

// The messages of the world that the session holds until the tick takes them, in the order they arrived, each
// with the epoch of its connection. While the map loads, they wait here instead of being dropped (IV.14.1 and
// IV.16 of the reference spec). The room is reserved once, so holding messages never asks for memory; a queue
// that passes the room grows instead of dropping anything, because the order matters (IV.14.6).

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mu::session {

// Counts the connections of the session: each new connection gets the next number, so an event of a
// connection that is already over is told apart and dropped (D-10 of the reference spec, item 5). From M4 on,
// each change of map also moves it.
struct SessionEpoch {
    std::uint32_t value = 0;

    friend constexpr bool operator==(SessionEpoch, SessionEpoch) noexcept = default;
};

class MessageQueue {
public:
    MessageQueue(std::size_t reservedBytes, std::size_t reservedMessages);

    // Copies the message to the end of the queue. False means it did not fit in the reserved room: the queue
    // grew, and kept the message.
    bool Push(SessionEpoch epoch, std::span<const std::byte> message);

    // Gives every message to `handler(SessionEpoch, std::span<const std::byte>)`, in order, then empties the
    // queue and keeps its room. The spans are valid only during the call. Gives how many.
    template <typename Handler>
    std::size_t Drain(Handler&& handler) {
        const std::span<const std::byte> bytes(m_bytes);
        for (const Record& record : m_records) {
            handler(record.epoch, bytes.subspan(record.offset, record.size));
        }
        const std::size_t count = m_records.size();
        Clear();
        return count;
    }

    void Clear() noexcept;

    [[nodiscard]] std::size_t Count() const noexcept { return m_records.size(); }
    [[nodiscard]] bool Empty() const noexcept { return m_records.empty(); }

private:
    struct Record {
        SessionEpoch epoch;
        std::size_t offset = 0;
        std::size_t size = 0;
    };

    std::vector<std::byte> m_bytes;
    std::vector<Record> m_records;
};

} // namespace mu::session
