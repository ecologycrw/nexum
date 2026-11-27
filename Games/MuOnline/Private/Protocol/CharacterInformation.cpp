#include "Mu/Protocol/CharacterInformation.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace mu::protocol {

nexum::core::Result<dialect::CharacterInformation::View>
ReadCharacterInformation(std::span<const std::byte> message, CharacterInformationBytes& whole) {
    if (message.size() >= whole.size()) {
        return dialect::CharacterInformation::View::Read(message);
    }
    if (message.size() < kShortestCharacterInformation) {
        return nexum::core::MakeError("the data of the character has " + std::to_string(message.size()) +
                                      " bytes; it needs at least " +
                                      std::to_string(kShortestCharacterInformation));
    }
    std::ranges::fill(whole, std::byte{0});
    std::ranges::copy(message, whole.begin());
    whole[1] = std::byte{static_cast<std::uint8_t>(whole.size())};
    return dialect::CharacterInformation::View::Read(whole);
}

} // namespace mu::protocol
