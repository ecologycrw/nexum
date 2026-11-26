// Fuzz target of the extended appearance (cmake/NexumFuzzing.cmake): any bytes, cut or longer than 27, give
// an appearance, never a crash, and the same first 27 bytes always give the same appearance.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>

#include "Mu/Protocol/ExtendedAppearance.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span<const std::uint8_t>(data, size));
    const auto whole = mu::protocol::ReadExtendedAppearance(bytes);
    const auto first = bytes.first(std::min(bytes.size(), mu::protocol::kExtendedAppearanceSize));
    if (mu::protocol::ReadExtendedAppearance(first) != whole) {
        std::abort();
    }
    return 0;
}
