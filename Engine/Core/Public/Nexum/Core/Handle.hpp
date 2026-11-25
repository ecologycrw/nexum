#pragma once

#include <cstdint>

namespace nexum::core {

// A reference to an object of type T kept in a HandlePool: the slot of the object and the generation the slot
// had when the object was added (D-07 and III.6 of the reference spec). Removing the object changes the
// generation of its slot, so an old handle is refused even after the slot holds a new object. Entities,
// resources and audio voices use it instead of pointers to things that can die.
//
// A handle of one type never stands for another type. The default handle refers to nothing: generations start
// at 1.
template <typename T>
struct Handle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const Handle&, const Handle&) = default;
};

} // namespace nexum::core
