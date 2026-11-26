#include "Mu/World/WorldEvent.hpp"

#include <algorithm>

namespace mu::world {

ObjectName ObjectName::FromBytes(std::span<const std::byte> bytes) noexcept {
    ObjectName name;
    const auto end = std::find(bytes.begin(), bytes.end(), std::byte{0});
    const auto length = static_cast<std::size_t>(end - bytes.begin());
    name.m_size = std::min(length, kLongest);
    for (std::size_t index = 0; index < name.m_size; ++index) {
        name.m_text[index] = static_cast<char>(bytes[index]);
    }
    return name;
}

} // namespace mu::world
