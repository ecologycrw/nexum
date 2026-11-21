#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Nexum/Core/Result.hpp"

namespace nexum::core {

// The memory of one frame, for the draw lists, the snapshot of the world and other data that lives a single
// frame (III.6 of the reference spec). All of it is reserved when the arena is created. Allocate hands out
// pieces one after the other, and Reset, at the start of the next frame, makes the whole arena free again, so
// the frame never asks the system for memory (INV-13). The same type serves as the arena of a tick.
//
// The arena never runs destructors: it only holds types that need none.
class FrameArena {
public:
    // An arena with room for `capacity` bytes.
    explicit FrameArena(std::size_t capacity);

    // A piece of `size` bytes whose address is a multiple of `alignment`, a power of two. The padding up to
    // that multiple also takes room. Returns an error when the arena is out of room.
    [[nodiscard]] Result<std::span<std::byte>> Allocate(std::size_t size, std::size_t alignment);

    // Room for `count` objects of type T, built in place with their default values.
    template <typename T>
    [[nodiscard]] Result<std::span<T>> AllocateArray(std::size_t count) {
        static_assert(std::is_trivially_destructible_v<T>, "the arena never runs destructors");
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            return MakeError("the frame arena cannot count " + std::to_string(count) + " objects");
        }
        auto storage = Reserve(count * sizeof(T), alignof(T));
        if (!storage.has_value()) {
            return MakeError(std::move(storage.error().message));
        }
        T* const first = static_cast<T*>(*storage);
        T* const last = std::uninitialized_value_construct_n(first, count);
        return std::span<T>(first, last);
    }

    // Makes the whole arena free again. What was handed out before must no longer be used.
    void Reset() noexcept;

    // How many bytes are taken, the padding included.
    [[nodiscard]] std::size_t Used() const noexcept;

    [[nodiscard]] std::size_t Capacity() const noexcept;

private:
    // Takes the room of a piece and returns where it starts, or nullptr for an empty piece.
    [[nodiscard]] Result<void*> Reserve(std::size_t size, std::size_t alignment);

    std::vector<std::byte> m_memory;
    std::size_t m_used = 0;
};

} // namespace nexum::core
