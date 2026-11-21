#include "Nexum/Core/Rgba8.hpp"

#include <string>

namespace nexum::core {

std::string ToString(const Rgba8& pixel) {
    return "(" + std::to_string(pixel.r) + ", " + std::to_string(pixel.g) + ", " + std::to_string(pixel.b) +
           ", " + std::to_string(pixel.a) + ")";
}

} // namespace nexum::core
