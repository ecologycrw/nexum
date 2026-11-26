#pragma once

// The messages of the way to the world, as the server sends them, for the tests of the session and of the
// online client. The order and the fields come from the protocol spec of the knowledge center
// (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.2 to 6.12) and from the examples of the plan of
// slice 1; the bytes of a real session come from the finding 2026-11-21-capturas-reais-sessao-106-3.md
// (section 3).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace session_messages {

using Bytes = std::vector<std::byte>;

[[nodiscard]] inline Bytes Make(std::initializer_list<std::uint8_t> values) {
    Bytes bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

[[nodiscard]] inline Bytes Text(std::string_view text, std::size_t size) {
    Bytes bytes(size);
    for (std::size_t index = 0; index < text.size(); ++index) {
        bytes[index] = static_cast<std::byte>(text[index]);
    }
    return bytes;
}

[[nodiscard]] inline Bytes Join(std::initializer_list<Bytes> parts) {
    Bytes joined;
    for (const auto& part : parts) {
        joined.insert(joined.end(), part.begin(), part.end());
    }
    return joined;
}

[[nodiscard]] inline Bytes Hello() {
    return Make({0xC1, 0x04, 0x00, 0x01});
}

// The list of a real session: 3 servers, with the ids 0, 1 and 2.
[[nodiscard]] inline Bytes ServerList() {
    return Make({0xC2, 0x00, 0x13, 0xF4, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                 0x02, 0x00, 0x00, 0x00});
}

// The address of the Game Server: 127.0.0.1 and the port, little-endian. A real session gives 45902 (0xB34E).
[[nodiscard]] inline Bytes ConnectionInfo(std::uint16_t port = 45902) {
    return Join({Make({0xC1, 0x16, 0xF4, 0x03}), Text("127.0.0.1", 16),
                 Make({static_cast<std::uint8_t>(port & 0xFFU), static_cast<std::uint8_t>(port >> 8U)})});
}

// The entry of a real session: success, the player id 0x0200 and the version "20404".
[[nodiscard]] inline Bytes GameServerEntered() {
    return Make({0xC1, 0x0C, 0xF1, 0x00, 0x01, 0x02, 0x00, 0x32, 0x30, 0x34, 0x30, 0x34});
}

[[nodiscard]] inline Bytes LoginResponse(std::uint8_t result) {
    return Make({0xC1, 0x05, 0xF1, 0x01, result});
}

// A list with these characters in the slots 0, 1...: 8 bytes, then 44 for each (section 6.9).
[[nodiscard]] inline Bytes CharacterList(std::initializer_list<std::string_view> names) {
    const auto count = static_cast<std::uint8_t>(names.size());
    Bytes list =
        Make({0xC1, static_cast<std::uint8_t>(8 + (44 * count)), 0xF3, 0x00, 0x00, 0x00, count, 0x00});
    std::uint8_t slot = 0;
    for (const std::string_view name : names) {
        Bytes character(44);
        character[0] = std::byte{slot++};
        const Bytes text = Text(name, 10);
        std::copy(text.begin(), text.end(), character.begin() + 1);
        character[42] = std::byte{0xFF}; // no guild
        list.insert(list.end(), character.begin(), character.end());
    }
    return list;
}

// The data of the character, of 93 bytes or fewer, at (145, 123) on the map 0, with the attack and magic
// speeds 29 and 29 (bytes 82 to 85, little-endian), as in a real session.
[[nodiscard]] inline Bytes CharacterInformation(std::size_t size = 93) {
    Bytes data(size);
    data[0] = std::byte{0xC3};
    data[1] = std::byte{static_cast<std::uint8_t>(size)};
    data[2] = std::byte{0xF3};
    data[3] = std::byte{0x03};
    data[4] = std::byte{145};
    data[5] = std::byte{123};
    if (size >= 86) {
        data[82] = std::byte{29};
        data[84] = std::byte{29};
    }
    return data;
}

// The player test0Dk coming into the view (C2 12), a message of the world of the first entry (section 6.12
// and the plan of slice 1).
[[nodiscard]] inline Bytes HeroEntered() {
    Bytes hero = Make({0xC2, 0x00, 0x36, 0x12, 0x00, 0x02, 0x91, 0x7B, 0x91, 0x7B, 0x00, 0x00,
                       0x1D, 0x00, 0x1D, 0x00, 0x74, 0x65, 0x73, 0x74, 0x30, 0x44, 0x6B});
    hero.resize(54);
    return hero;
}

} // namespace session_messages
