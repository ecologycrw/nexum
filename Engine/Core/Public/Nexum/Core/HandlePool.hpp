#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Nexum/Core/Handle.hpp"
#include "Nexum/Core/Result.hpp"

namespace nexum::core {

// A fixed number of objects of type T, reached by Handle<T> (III.6 of the reference spec). All the memory is
// reserved when the pool is created: it never grows, and a full pool refuses a new object with an error. A
// system with another policy, such as dropping its oldest particle, applies it on top of that error.
//
// An object stays at the same address until it is removed, so the pointer from Find stays valid until then.
template <typename T>
class HandlePool {
public:
    // A pool with room for `capacity` objects.
    explicit HandlePool(std::uint32_t capacity) : m_slots(capacity) {
        m_freeSlots.reserve(capacity);
        // The free slots are taken from the back: slot 0 goes first.
        for (std::uint32_t index = capacity; index > 0; --index) {
            m_freeSlots.push_back(index - 1);
        }
    }

    // Moves the object into a free slot and returns its handle, or an error when the pool is full.
    [[nodiscard]] Result<Handle<T>> Add(T object) {
        if (m_freeSlots.empty()) {
            return MakeError("the pool is full (" + std::to_string(Capacity()) + " objects)");
        }
        const std::uint32_t index = m_freeSlots.back();
        m_freeSlots.pop_back();
        Slot& slot = m_slots[index];
        slot.object.emplace(std::move(object));
        ++m_size;
        return Handle<T>{.index = index, .generation = slot.generation};
    }

    // Removes the object of the handle. Returns false, and changes nothing, when the handle is refused.
    bool Remove(Handle<T> handle) noexcept {
        Slot* const slot = MatchingSlot(*this, handle);
        if (slot == nullptr || !slot->object.has_value()) {
            return false;
        }
        slot->object.reset();
        --m_size;
        // A slot whose generation would start over is never used again, so that no old handle can ever match
        // it.
        if (slot->generation != kLastGeneration) {
            ++slot->generation;
            m_freeSlots.push_back(handle.index);
        }
        return true;
    }

    // The object of the handle, or nullptr when the handle is refused: removed, from another slot, or the
    // default.
    [[nodiscard]] T* Find(Handle<T> handle) noexcept {
        Slot* const slot = MatchingSlot(*this, handle);
        return slot != nullptr && slot->object.has_value() ? &*slot->object : nullptr;
    }

    [[nodiscard]] const T* Find(Handle<T> handle) const noexcept {
        const Slot* const slot = MatchingSlot(*this, handle);
        return slot != nullptr && slot->object.has_value() ? &*slot->object : nullptr;
    }

    [[nodiscard]] bool Contains(Handle<T> handle) const noexcept { return Find(handle) != nullptr; }

    // How many objects the pool holds now.
    [[nodiscard]] std::uint32_t Size() const noexcept { return m_size; }

    [[nodiscard]] std::uint32_t Capacity() const noexcept {
        return static_cast<std::uint32_t>(m_slots.size());
    }

private:
    static constexpr std::uint32_t kLastGeneration = std::numeric_limits<std::uint32_t>::max();

    struct Slot {
        std::optional<T> object;
        // Starts at 1, so the default handle, with generation 0, never matches.
        std::uint32_t generation = 1;
    };

    // The slot of the handle when its generation still matches, for the const and the non-const pool alike.
    // The caller still checks that the slot holds an object: a slot that will never be used again keeps its
    // last generation.
    template <typename Pool>
    [[nodiscard]] static auto* MatchingSlot(Pool& pool, Handle<T> handle) noexcept {
        auto* const slot = handle.index < pool.m_slots.size() ? &pool.m_slots[handle.index] : nullptr;
        return slot != nullptr && slot->generation == handle.generation ? slot : nullptr;
    }

    std::vector<Slot> m_slots;
    std::vector<std::uint32_t> m_freeSlots;
    std::uint32_t m_size = 0;
};

} // namespace nexum::core
