// Fuzz target of ParseVersion (cmake/NexumFuzzing.cmake): any text gives a version or an error, never a
// crash, and a version that was read reads back the same after it is written again.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string>

#include "Nexum/Core/Version.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::span<const std::uint8_t> bytes(data, size);
    std::string text(bytes.size(), '\0');
    std::transform(bytes.begin(), bytes.end(), text.begin(),
                   [](std::uint8_t byte) { return static_cast<char>(byte); });
    const auto version = nexum::core::ParseVersion(text);
    if (version) {
        const auto again = nexum::core::ParseVersion(nexum::core::ToString(*version));
        if (!again || *again != *version) {
            std::abort();
        }
    }
    return 0;
}
