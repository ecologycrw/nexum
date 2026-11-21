#pragma once

#include <cstdint>

namespace nexum::core {

// One pixel with 8 bits per channel, in the order red, green, blue and alpha.
struct Rgba8 {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;

    friend bool operator==(const Rgba8&, const Rgba8&) = default;
};

} // namespace nexum::core
