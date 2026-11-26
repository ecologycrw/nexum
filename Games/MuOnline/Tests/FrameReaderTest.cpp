// The framing of the messages (CA-01). The examples are those of the plan of slice 1, in "Suficiência das
// descrições", computed from the protocol spec of the knowledge center, section 3, and from a real capture.
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/FrameReader.hpp"

namespace {

using mu::protocol::FrameReader;

[[nodiscard]] std::vector<std::byte> Bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

// Pushes the bytes in pieces of `pieceSize` and collects the messages.
struct Collected {
    std::vector<std::vector<std::byte>> messages;
    std::string error;
};

[[nodiscard]] Collected PushInPieces(FrameReader& reader, const std::vector<std::byte>& bytes,
                                     std::size_t pieceSize) {
    Collected collected;
    for (std::size_t start = 0; start < bytes.size(); start += pieceSize) {
        const std::size_t size = std::min(pieceSize, bytes.size() - start);
        const auto pushed = reader.Push(std::span<const std::byte>(bytes).subspan(start, size),
                                        [&](std::span<const std::byte> message) {
                                            collected.messages.emplace_back(message.begin(), message.end());
                                        });
        if (!pushed) {
            collected.error = pushed.error().message;
            break;
        }
    }
    return collected;
}

// A C1, a C2, a C3 and a C4 one after the other: the C2 comes from the server list of a real capture.
const std::vector<std::byte> kFourKinds = Bytes({
    0xC1, 0x04, 0x00, 0x01,                                                 // Hello
    0xC2, 0x00, 0x13, 0xF4, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, // server list, part 1
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,                               // server list, part 2
    0xC3, 0x05, 0xF1, 0x02, 0x00,                                           // a C3 of 5 bytes
    0xC4, 0x00, 0x06, 0xF3, 0x10, 0x00,                                     // a C4 of 6 bytes
});

} // namespace

TEST_CASE("[0001-CA-01] two messages in one piece become two messages") {
    FrameReader reader;
    const auto collected = PushInPieces(reader, Bytes({0xC1, 0x04, 0x00, 0x01, 0xC1, 0x04, 0xF4, 0x06}), 8);
    REQUIRE(collected.error.empty());
    REQUIRE(collected.messages.size() == 2);
    CHECK(collected.messages[0] == Bytes({0xC1, 0x04, 0x00, 0x01}));
    CHECK(collected.messages[1] == Bytes({0xC1, 0x04, 0xF4, 0x06}));
    CHECK(reader.PendingBytes() == 0);
}

TEST_CASE("[0001-CA-01] a message cut after its first two bytes comes out only when its last byte arrives") {
    FrameReader reader;
    const auto list = std::vector<std::byte>(kFourKinds.begin() + 4, kFourKinds.begin() + 23);
    REQUIRE(list.size() == 19);
    std::vector<std::vector<std::byte>> messages;
    const auto collect = [&](std::span<const std::byte> message) {
        messages.emplace_back(message.begin(), message.end());
    };

    REQUIRE(reader.Push(std::span<const std::byte>(list).first(2), collect).has_value());
    CHECK(messages.empty());
    REQUIRE(reader.Push(std::span<const std::byte>(list).subspan(2, 16), collect).has_value());
    CHECK(messages.empty());
    CHECK(reader.PendingBytes() == 18);
    REQUIRE(reader.Push(std::span<const std::byte>(list).subspan(18), collect).has_value());
    REQUIRE(messages.size() == 1);
    CHECK(messages[0] == list);
}

TEST_CASE("[0001-CA-01] the four kinds of message come out whole in pieces of every size, down to one byte") {
    for (std::size_t pieceSize = 1; pieceSize <= kFourKinds.size(); ++pieceSize) {
        CAPTURE(pieceSize);
        FrameReader reader;
        const auto collected = PushInPieces(reader, kFourKinds, pieceSize);
        REQUIRE(collected.error.empty());
        REQUIRE(collected.messages.size() == 4);
        CHECK(collected.messages[0].size() == 4);
        CHECK(collected.messages[1].size() == 19);
        CHECK(collected.messages[2] == Bytes({0xC3, 0x05, 0xF1, 0x02, 0x00}));
        CHECK(collected.messages[3] == Bytes({0xC4, 0x00, 0x06, 0xF3, 0x10, 0x00}));
        CHECK(reader.PendingBytes() == 0);
    }
}

TEST_CASE("[0001-CA-01] an unknown first byte closes the connection with the reason") {
    for (const std::size_t pieceSize : {std::size_t{1}, std::size_t{4}}) {
        FrameReader reader;
        const auto collected = PushInPieces(reader, Bytes({0xC5, 0x04, 0x00, 0x01}), pieceSize);
        CHECK(collected.messages.empty());
        CHECK(collected.error == "the first byte C5 is not C1, C2, C3 or C4");
    }
    // After a whole message, the next one is checked the same way.
    FrameReader reader;
    const auto collected = PushInPieces(reader, Bytes({0xC1, 0x04, 0x00, 0x01, 0x00, 0x01}), 6);
    CHECK(collected.messages.size() == 1);
    CHECK(collected.error == "the first byte 00 is not C1, C2, C3 or C4");
}

TEST_CASE("[0001-CA-01] a length of 0 closes the connection with the reason") {
    for (const auto& bytes : {Bytes({0xC1, 0x00, 0xF4}), Bytes({0xC2, 0x00, 0x00, 0xF4}), Bytes({0xC3, 0x00}),
                              Bytes({0xC4, 0x00, 0x00})}) {
        for (const std::size_t pieceSize : {std::size_t{1}, bytes.size()}) {
            FrameReader reader;
            CHECK(PushInPieces(reader, bytes, pieceSize).error == "a message declares the length 0");
        }
    }
}

TEST_CASE(
    "[0001-CA-01] a length shorter than the header and the code closes the connection with the reason") {
    FrameReader shortC1;
    CHECK(PushInPieces(shortC1, Bytes({0xC1, 0x02, 0xF4}), 1).error ==
          "a message declares 2 bytes, less than its header and code");
    FrameReader shortC4;
    CHECK(PushInPieces(shortC4, Bytes({0xC4, 0x00, 0x03, 0xF3}), 4).error ==
          "a message declares 3 bytes, less than its header and code");
    // The smallest valid messages: the header and the code.
    FrameReader smallest;
    const auto collected = PushInPieces(smallest, Bytes({0xC1, 0x03, 0xF4, 0xC2, 0x00, 0x04, 0xF4}), 1);
    CHECK(collected.error.empty());
    CHECK(collected.messages.size() == 2);
}

TEST_CASE("[0001-CA-01] the largest C2 message, of 65535 bytes, comes out whole") {
    std::vector<std::byte> bytes(0xFFFF, std::byte{0x5A});
    bytes[0] = std::byte{0xC2};
    bytes[1] = std::byte{0xFF};
    bytes[2] = std::byte{0xFF};
    FrameReader reader;
    const auto collected = PushInPieces(reader, bytes, 1000);
    REQUIRE(collected.error.empty());
    REQUIRE(collected.messages.size() == 1);
    CHECK(collected.messages[0] == bytes);
}
