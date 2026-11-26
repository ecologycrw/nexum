// The packets as the log shows them (CA-12): the user and the password of the login sent are hidden, with or
// without the public Xor3 on them (docs/specs/protocolo/protocolo-106-3-m2.md, sections 4.4 and 6.6).
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/LoginMessage.hpp"
#include "Mu/Protocol/PacketLog.hpp"

namespace {

using mu::protocol::FormatPacket;
using mu::protocol::PacketDirection;

[[nodiscard]] std::string Stars(int count) {
    std::string stars;
    for (int index = 0; index < count; ++index) {
        stars += " **";
    }
    return stars;
}

} // namespace

TEST_CASE("[0001-CA-12] the log of a login sent hides the 30 bytes of the user and the password") {
    mu::protocol::LoginMessage login{};
    REQUIRE(mu::protocol::WriteLogin("test0", "test0", 0x08B75ACE, login).has_value());
    const std::string text = FormatPacket(PacketDirection::Sent, login);
    // The time, big-endian, and the version "20404" stay; the serial follows.
    CHECK(
        text.starts_with("sent 60 bytes: C3 3C F1 01" + Stars(30) + " 08 B7 5A CE 32 30 34 30 34 4E 45 58"));
    CHECK(text.find("88 AA D8") == std::string::npos);
    CHECK(text.find("74 65 73 74") == std::string::npos);
}

TEST_CASE("[0001-CA-12] the answer to the login, which has no credentials, is shown whole") {
    const std::vector<std::byte> response{std::byte{0xC1}, std::byte{0x05}, std::byte{0xF1}, std::byte{0x01},
                                          std::byte{0x00}};
    CHECK(FormatPacket(PacketDirection::Received, response) == "received 5 bytes: C1 05 F1 01 00");
}

TEST_CASE("the log shows the first 64 bytes of a long packet and counts the rest") {
    std::vector<std::byte> list(100, std::byte{0xAB});
    const std::string text = FormatPacket(PacketDirection::Received, list);
    CHECK(text.starts_with("received 100 bytes: AB AB"));
    CHECK(text.ends_with(" AB ... (36 more)"));
}
