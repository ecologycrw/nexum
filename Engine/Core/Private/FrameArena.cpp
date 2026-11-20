#include "Nexum/Core/FrameArena.hpp"

#include <bit>
#include <memory>
#include <string>
#include <utility>

namespace nexum::core {

FrameArena::FrameArena(std::size_t capacity) : m_memory(capacity) {}

Result<std::span<std::byte>> FrameArena::Allocate(std::size_t size, std::size_t alignment) {
    auto start = Reserve(size, alignment);
    if (!start.has_value()) {
        return MakeError(std::move(start.error().message));
    }
    return std::span<std::byte>(static_cast<std::byte*>(*start), size);
}

Result<void*> FrameArena::Reserve(std::size_t size, std::size_t alignment) {
    if (!std::has_single_bit(alignment)) {
        return MakeError("the alignment must be a power of two, not " + std::to_string(alignment));
    }
    if (size == 0) {
        return nullptr;
    }

    // The alignment is of the address itself, which may be larger than the alignment the vector guarantees.
    void* next = m_memory.data() + m_used;
    std::size_t left = m_memory.size() - m_used;
    if (std::align(alignment, size, next, left) == nullptr) {
        return MakeError("the frame arena is out of room: " + std::to_string(size) +
                         " bytes asked with alignment " + std::to_string(alignment) + ", " +
                         std::to_string(m_memory.size() - m_used) + " of " + std::to_string(m_memory.size()) +
                         " left");
    }

    m_used = static_cast<std::size_t>(static_cast<std::byte*>(next) - m_memory.data()) + size;
    return next;
}

void FrameArena::Reset() noexcept {
    m_used = 0;
}

std::size_t FrameArena::Used() const noexcept {
    return m_used;
}

std::size_t FrameArena::Capacity() const noexcept {
    return m_memory.size();
}

} // namespace nexum::core
