#pragma once

// The extended item of the 106.3 dialect: from 5 to 15 bytes, which become a neutral item description. The
// layout and the size rule come from the protocol spec of the knowledge center
// (docs/specs/protocolo/protocolo-106-3-m2.md, section 7.1) and from the finding
// 2026-11-21-item-estendido.md, which has the vectors.

#include <cstddef>
#include <span>

#include "Mu/Items/Item.hpp"

namespace mu::protocol {

struct ExtendedItem {
    items::Item item;
    // How many bytes the item takes: the size its flags give, but never more than the bytes that are there.
    std::size_t size = 0;
};

// Reads the item at the start of the bytes. A byte past the end reads as 0, as the spec says, so a cut item
// reads as an item without the parts that are missing, and nothing is read past the end.
[[nodiscard]] ExtendedItem ReadExtendedItem(std::span<const std::byte> bytes) noexcept;

} // namespace mu::protocol
