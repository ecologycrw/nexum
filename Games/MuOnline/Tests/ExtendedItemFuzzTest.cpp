// Fuzz target of the extended item (cmake/NexumFuzzing.cmake): any bytes give an item, never a crash; an item
// never takes more bytes than there are, and a row of items, read one after the other as in the inventory,
// always moves forward.
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>

#include "Mu/Protocol/ExtendedItem.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span<const std::uint8_t>(data, size));
    for (std::size_t start = 0; start < bytes.size();) {
        const auto read = mu::protocol::ReadExtendedItem(bytes.subspan(start));
        if (read.size == 0 || read.size > bytes.size() - start) {
            std::abort();
        }
        start += read.size;
    }
    static_cast<void>(mu::protocol::ReadExtendedItem(bytes));
    return 0;
}
