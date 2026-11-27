#pragma once

// The digest of the state of the view of the world, version 1 (kDigestVersion in Mu/Recording/Recording.hpp,
// and the plan of slice 2, "A impressão do estado, versão 1"): the 64-bit FNV-1a (Nexum/Core/Fnv1a.hpp) over
// the fields below, in this order, each number in little-endian with its own size and each flag as one byte,
// 0 or 1:
//
// 1. the hero: loaded, id, map, position (x and y), the 6 values of HeroStats and the 4 of HeroMaxima, in the
//    order of their declaration;
// 2. the sum of the digests of the objects in view (8 bytes). Each object has the digest of its id, kind,
//    position, target, rotation, name (its size in one byte, then its bytes), appearance (every field, in the
//    order of the declaration, with the 7 worn items, the wings and the pet), count of effects and number of
//    the type;
// 3. the sum of the digests of the entries of the inventory (8 bytes): the slot and every field of the item,
//    in the order of the declaration, with the 15 sockets;
// 4. the count of objects and the count of entries (4 bytes each);
// 5. the count of the changes of the terrain and the count of the ignored events (8 bytes each).
//
// The sums are taken modulo 2^64, so the order of the tables does not change the digest, and nothing is
// sorted: taking a digest never asks for memory (INV-13). The recording keeps the digest at each checkpoint,
// and the replay compares it. When the view changes shape (the entities of M4, TD-0029), the digest gains a
// version 2.

#include <cstdint>

#include "Mu/World/WorldView.hpp"

namespace mu::recording {

[[nodiscard]] std::uint64_t StateDigestOf(const world::WorldView& view) noexcept;

} // namespace mu::recording
