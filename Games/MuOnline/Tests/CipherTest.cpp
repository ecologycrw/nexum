// The encryption (CA-02), with the vectors of the knowledge center: V1 to V9 of the finding
// 2026-11-21-cifra-simplemodulus-xor32-xor3.md (section "Vetores de teste"), and the vectors of real sessions
// of the finding 2026-11-21-capturas-reais-sessao-106-3.md (section 3), where the original client encrypted
// and the knowledge center decrypted and encrypted again.
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/Cipher.hpp"

namespace {

using mu::protocol::kClientToServerDecryptKey;
using mu::protocol::kClientToServerEncryptKey;
using mu::protocol::kServerToClientDecryptKey;
using mu::protocol::kServerToClientEncryptKey;
using mu::protocol::PacketDecryptor;
using mu::protocol::PacketEncryptor;

[[nodiscard]] std::vector<std::byte> Bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

// What the client puts on the wire: Xor32 on the plain message, then SimpleModulus (section 4.1).
[[nodiscard]] std::vector<std::byte> ClientSends(PacketEncryptor& encryptor, std::vector<std::byte> plain) {
    mu::protocol::Xor32Encrypt(plain);
    std::vector<std::byte> wire((plain.size() * 2) + mu::protocol::kEncryptedBlockSize);
    const auto written = encryptor.Encrypt(plain, wire);
    REQUIRE(written.has_value());
    wire.resize(*written);
    return wire;
}

[[nodiscard]] std::vector<std::byte> Encrypt(PacketEncryptor& encryptor,
                                             const std::vector<std::byte>& plain) {
    std::vector<std::byte> wire((plain.size() * 2) + mu::protocol::kEncryptedBlockSize);
    const auto written = encryptor.Encrypt(plain, wire);
    REQUIRE(written.has_value());
    wire.resize(*written);
    return wire;
}

[[nodiscard]] std::vector<std::byte> Decrypt(PacketDecryptor& decryptor, const std::vector<std::byte>& wire) {
    std::vector<std::byte> plain(wire.size());
    const auto written = decryptor.Decrypt(wire, plain);
    REQUIRE(written.has_value());
    plain.resize(*written);
    return plain;
}

// Moves the counter of a direction forward by sending other C3 messages first.
void Advance(PacketEncryptor& encryptor, int count) {
    for (int index = 0; index < count; ++index) {
        static_cast<void>(Encrypt(encryptor, Bytes({0xC3, 0x04, 0xF1, 0x00})));
    }
}

void Advance(PacketDecryptor& decryptor, PacketEncryptor& serverSide, int count) {
    for (int index = 0; index < count; ++index) {
        static_cast<void>(Decrypt(decryptor, Encrypt(serverSide, Bytes({0xC3, 0x04, 0xF1, 0x00}))));
    }
}

// V2: the ping of the client, in the clear.
const std::vector<std::byte> kPing =
    Bytes({0xC3, 0x0C, 0x0E, 0x00, 0x01, 0x33, 0xFE, 0x27, 0x00, 0x00, 0x00, 0x00});

// V4: the ping on the wire; only the first block changes with the counter, the second one is always this.
[[nodiscard]] std::vector<std::byte> PingOnTheWire(std::initializer_list<std::uint8_t> firstBlock) {
    auto wire = Bytes({0xC3, 0x18});
    for (const std::uint8_t value : firstBlock) {
        wire.push_back(std::byte{value});
    }
    for (const std::byte value : Bytes({0xFA, 0x64, 0x64, 0x63, 0x5B, 0xA1, 0xB3, 0x61, 0x05, 0xB5, 0x8B})) {
        wire.push_back(value);
    }
    return wire;
}

// V6: the message C3 0A F3 03 11 22 33 44 55 66 from the server, on the wire.
const std::vector<std::byte> kServerMessage =
    Bytes({0xC3, 0x0A, 0xF3, 0x03, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66});

[[nodiscard]] std::vector<std::byte> ServerMessageOnTheWire(std::initializer_list<std::uint8_t> firstBlock) {
    auto wire = Bytes({0xC3, 0x18});
    for (const std::uint8_t value : firstBlock) {
        wire.push_back(std::byte{value});
    }
    for (const std::byte value : Bytes({0x6B, 0xBD, 0x2E, 0x1C, 0x94, 0x78, 0x60, 0x6F, 0x24, 0xA2, 0x9E})) {
        wire.push_back(value);
    }
    return wire;
}

} // namespace

TEST_CASE("[0001-CA-02] V1: one block of 8 bytes becomes 11, and back") {
    const auto plain = Bytes({0x00, 0x0E, 0x00, 0x01, 0x33, 0xFE, 0x27, 0x00});
    std::array<std::byte, 11> encrypted{};
    mu::protocol::EncryptBlock(std::span<const std::byte, 8>(plain.data(), 8), 8, kClientToServerEncryptKey,
                               encrypted);
    const auto expected = Bytes({0xC8, 0x31, 0x22, 0x99, 0x8E, 0x94, 0x81, 0x19, 0x34, 0x28, 0x1D});
    CHECK(std::vector<std::byte>(encrypted.begin(), encrypted.end()) == expected);

    std::array<std::byte, 8> decrypted{};
    const auto realBytes = mu::protocol::DecryptBlock(encrypted, kClientToServerDecryptKey, decrypted);
    REQUIRE(realBytes.has_value());
    CHECK(*realBytes == 8);
    CHECK(std::vector<std::byte>(decrypted.begin(), decrypted.end()) == plain);
}

TEST_CASE(
    "[0001-CA-02] V2: the client puts the ping through Xor32 and then SimpleModulus with the counter 0") {
    auto afterXor32 = kPing;
    mu::protocol::Xor32Encrypt(afterXor32);
    CHECK(afterXor32 == Bytes({0xC3, 0x0C, 0x0E, 0xF0, 0xE9, 0xF9, 0xC2, 0x46, 0x8C, 0xBF, 0x7E, 0xB2}));

    PacketEncryptor encryptor(kClientToServerEncryptKey);
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0xBC, 0x9D, 0x35, 0xE2, 0xA2, 0x21, 0x22, 0x91, 0x5C, 0x2B, 0x1E}));
    CHECK(encryptor.Counter() == 1);

    // The server undoes it in the opposite order.
    PacketDecryptor serverSide(kClientToServerDecryptKey);
    auto plain = Decrypt(serverSide,
                         PingOnTheWire({0xBC, 0x9D, 0x35, 0xE2, 0xA2, 0x21, 0x22, 0x91, 0x5C, 0x2B, 0x1E}));
    mu::protocol::Xor32Decrypt(plain);
    CHECK(plain == kPing);
}

TEST_CASE("[0001-CA-02] V3: a C1 of the client goes only through Xor32, from the byte after the code") {
    for (const auto& [plain, wire] : {
             std::pair{Bytes({0xC1, 0x04, 0xF3, 0x00}), Bytes({0xC1, 0x04, 0xF3, 0x0D})},
             std::pair{Bytes({0xC1, 0x05, 0xF3, 0x03, 0x00}), Bytes({0xC1, 0x05, 0xF3, 0x0E, 0x16})},
             std::pair{Bytes({0xC1, 0x06, 0x11, 0x01, 0x02, 0x03}),
                       Bytes({0xC1, 0x06, 0x11, 0xEE, 0xF4, 0xD4})},
         }) {
        PacketEncryptor encryptor(kClientToServerEncryptKey);
        CHECK(ClientSends(encryptor, plain) == wire);
        CHECK(encryptor.Counter() == 0);
        auto back = wire;
        mu::protocol::Xor32Decrypt(back);
        CHECK(back == plain);
    }
}

TEST_CASE(
    "[0001-CA-02] V4: the same ping with the counters 0, 1, 2, 3, 254, 255 and 0 again, and a C1 in between "
    "spends no counter") {
    PacketEncryptor encryptor(kClientToServerEncryptKey);
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0xBC, 0x9D, 0x35, 0xE2, 0xA2, 0x21, 0x22, 0x91, 0x5C, 0x2B, 0x1E}));
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0x24, 0x98, 0x0C, 0x68, 0x47, 0x65, 0x41, 0x35, 0x68, 0x2A, 0x1F}));
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0x2C, 0xC5, 0x61, 0x8F, 0x25, 0x4D, 0xF2, 0x74, 0x30, 0x29, 0x1C}));
    CHECK(ClientSends(encryptor, Bytes({0xC1, 0x04, 0xF3, 0x00})) == Bytes({0xC1, 0x04, 0xF3, 0x0D}));
    CHECK(ClientSends(encryptor, Bytes({0xC2, 0x00, 0x05, 0xF3, 0x00})).size() == 5);
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0xCC, 0xB5, 0x17, 0x04, 0xD0, 0xC4, 0x10, 0xC1, 0xA5, 0x28, 0x1D}));
    Advance(encryptor, 250);
    CHECK(encryptor.Counter() == 254);
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0xB2, 0x16, 0x19, 0xF6, 0x5B, 0x03, 0x41, 0xF8, 0xD0, 0xD5, 0xE0}));
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0x62, 0x62, 0x01, 0x1E, 0x80, 0xC4, 0x10, 0xBB, 0x28, 0xD4, 0xE1}));
    CHECK(encryptor.Counter() == 0);
    CHECK(ClientSends(encryptor, kPing) ==
          PingOnTheWire({0xBC, 0x9D, 0x35, 0xE2, 0xA2, 0x21, 0x22, 0x91, 0x5C, 0x2B, 0x1E}));
}

TEST_CASE(
    "[0001-CA-02] V5: the client decrypts C3 messages of the server, of 1 to 3 blocks, with the counter 0") {
    {
        PacketDecryptor decryptor(kServerToClientDecryptKey);
        CHECK(Decrypt(decryptor, Bytes({0xC3, 0x0D, 0x68, 0xCE, 0x2F, 0xC3, 0xCB, 0xD2, 0xE0, 0x8C, 0x81,
                                        0x36, 0x08})) == Bytes({0xC3, 0x04, 0xF1, 0x01}));
        CHECK(decryptor.ExpectedCounter() == 1);
    }
    {
        PacketDecryptor decryptor(kServerToClientDecryptKey);
        CHECK(Decrypt(decryptor, ServerMessageOnTheWire({0x8E, 0x4F, 0x02, 0x56, 0x4B, 0xF2, 0xA3, 0xA7, 0x94,
                                                         0x2C, 0x19})) == kServerMessage);
    }
    {
        std::vector<std::byte> plain = Bytes({0xC3, 0x14, 0x1C, 0x0F});
        for (std::uint8_t value = 0x01; value <= 0x10; ++value) {
            plain.push_back(std::byte{value});
        }
        PacketDecryptor decryptor(kServerToClientDecryptKey);
        CHECK(Decrypt(decryptor,
                      Bytes({0xC3, 0x23, 0x52, 0xB9, 0x2C, 0xE9, 0x49, 0x77, 0x52, 0x1C, 0x9C, 0xDF,
                             0xEA, 0xC8, 0x4C, 0x31, 0x8A, 0xDD, 0x9B, 0x65, 0x81, 0x94, 0xCD, 0xF8,
                             0xA5, 0x94, 0x23, 0x7B, 0x5D, 0x3C, 0x51, 0x59, 0x04, 0xD7, 0xE9})) == plain);
    }
}

TEST_CASE("[0001-CA-02] V6: the client decrypts the same server message with the counters 0, 1, 2, 255 and 0 "
          "again") {
    PacketDecryptor decryptor(kServerToClientDecryptKey);
    PacketEncryptor serverSide(kServerToClientEncryptKey);
    for (const auto& wire : {
             ServerMessageOnTheWire({0x8E, 0x4F, 0x02, 0x56, 0x4B, 0xF2, 0xA3, 0xA7, 0x94, 0x2C, 0x19}),
             ServerMessageOnTheWire({0xA5, 0xE2, 0x36, 0x5D, 0x9F, 0x99, 0x03, 0x14, 0x2A, 0x2D, 0x18}),
             ServerMessageOnTheWire({0x5D, 0x4F, 0x26, 0xAE, 0xC4, 0x3F, 0x94, 0xCD, 0x3E, 0x2E, 0x1B}),
         }) {
        CHECK(Encrypt(serverSide, kServerMessage) == wire);
        CHECK(Decrypt(decryptor, wire) == kServerMessage);
    }
    Advance(decryptor, serverSide, 252);
    const auto at255 =
        ServerMessageOnTheWire({0x3F, 0x99, 0x3D, 0x38, 0x47, 0x38, 0xD5, 0x90, 0x28, 0xD3, 0xE6});
    CHECK(decryptor.ExpectedCounter() == 255);
    CHECK(Encrypt(serverSide, kServerMessage) == at255);
    CHECK(Decrypt(decryptor, at255) == kServerMessage);
    const auto again =
        ServerMessageOnTheWire({0x8E, 0x4F, 0x02, 0x56, 0x4B, 0xF2, 0xA3, 0xA7, 0x94, 0x2C, 0x19});
    CHECK(Encrypt(serverSide, kServerMessage) == again);
    CHECK(Decrypt(decryptor, again) == kServerMessage);
}

TEST_CASE("[0001-CA-02] V7: the client decrypts a C4 of the server, with its header of 3 bytes") {
    const auto plain = Bytes({0xC4, 0x00, 0x0C, 0xF3, 0x10, 0x01, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55});
    const auto wire = Bytes({0xC4, 0x00, 0x19, 0xA0, 0x25, 0x34, 0x9C, 0x52, 0x74, 0x12, 0x13, 0x31, 0x2F,
                             0x1A, 0x3F, 0x65, 0x21, 0xFF, 0xDE, 0xF0, 0xB1, 0x80, 0x88, 0xD6, 0xE9});
    PacketDecryptor decryptor(kServerToClientDecryptKey);
    CHECK(Decrypt(decryptor, wire) == plain);
    PacketEncryptor serverSide(kServerToClientEncryptKey);
    CHECK(Encrypt(serverSide, plain) == wire);
}

TEST_CASE("[0001-CA-02] V8: Xor3 on the user field \"test0\", the zeros included") {
    auto field = Bytes({0x74, 0x65, 0x73, 0x74, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00});
    mu::protocol::Xor3(field);
    CHECK(field == Bytes({0x88, 0xAA, 0xD8, 0x88, 0xFF, 0xAB, 0xFC, 0xCF, 0xAB, 0xFC}));
    mu::protocol::Xor3(field);
    CHECK(field == Bytes({0x74, 0x65, 0x73, 0x74, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}));
}

TEST_CASE("[0001-CA-02] V9: a wrong counter closes the connection, with no attempt to resynchronize") {
    // The ping encrypted with the counter 5, given to a decryptor that expects 0.
    PacketEncryptor client(kClientToServerEncryptKey);
    Advance(client, 5);
    const auto ping = ClientSends(client, kPing);
    PacketDecryptor serverSide(kClientToServerDecryptKey);
    std::vector<std::byte> plain(ping.size());
    const auto refused = serverSide.Decrypt(ping, plain);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the counter of the message is 5, not 0");

    // The first message of V5 received while the client expects the counter 1.
    PacketDecryptor decryptor(kServerToClientDecryptKey);
    PacketEncryptor server(kServerToClientEncryptKey);
    Advance(decryptor, server, 1);
    const auto wire = Bytes({0xC3, 0x0D, 0x68, 0xCE, 0x2F, 0xC3, 0xCB, 0xD2, 0xE0, 0x8C, 0x81, 0x36, 0x08});
    std::vector<std::byte> out(wire.size());
    CHECK(decryptor.Decrypt(wire, out).error().message == "the counter of the message is 0, not 1");
}

TEST_CASE("[0001-CA-02] real sessions: the pings of the client with the counters 1, 7 and 10, and its logout "
          "with 26") {
    struct Vector {
        int counter;
        std::vector<std::byte> plain;
        std::vector<std::byte> wire;
    };
    const std::vector<Vector> vectors{
        {1, Bytes({0xC3, 0x0C, 0x0E, 0x00, 0x8D, 0x83, 0xB5, 0x08, 0x00, 0x00, 0x00, 0x00}),
         Bytes({0xC3, 0x18, 0xEA, 0xD5, 0x0A, 0x20, 0x87, 0x25, 0x92, 0x78, 0xE4, 0xED,
                0xD8, 0xC0, 0x2C, 0x67, 0x3B, 0x6D, 0xE2, 0x41, 0xF4, 0x38, 0xED, 0xD3})},
        {7, Bytes({0xC3, 0x0C, 0x0E, 0x00, 0x7C, 0x58, 0xB7, 0x08, 0x00, 0x00, 0x00, 0x00}),
         Bytes({0xC3, 0x18, 0xC4, 0x51, 0x59, 0xAE, 0xD4, 0x68, 0xD0, 0x74, 0x50, 0x18,
                0x2D, 0xAE, 0x06, 0x06, 0xF6, 0xD2, 0x82, 0xA2, 0xE8, 0x1D, 0xC5, 0xFB})},
        {10, Bytes({0xC3, 0x0C, 0x0E, 0x00, 0x9C, 0xA6, 0xB7, 0x08, 0x1D, 0x00, 0x1D, 0x00}),
         Bytes({0xC3, 0x18, 0x3A, 0xA1, 0x0D, 0x92, 0x27, 0x0E, 0x93, 0x60, 0x70, 0xE8,
                0xDD, 0xB8, 0x07, 0x32, 0xAE, 0x4B, 0x02, 0xB2, 0xE1, 0x60, 0xC6, 0xF8})},
        {26, Bytes({0xC3, 0x05, 0xF1, 0x02, 0x00}),
         Bytes({0xC3, 0x0D, 0xC9, 0xF6, 0x15, 0xE4, 0x0F, 0x81, 0x82, 0x16, 0x00, 0x32, 0x0B})},
    };
    for (const auto& vector : vectors) {
        CAPTURE(vector.counter);
        PacketEncryptor encryptor(kClientToServerEncryptKey);
        Advance(encryptor, vector.counter);
        CHECK(ClientSends(encryptor, vector.plain) == vector.wire);
    }
}

TEST_CASE(
    "[0001-CA-02] real sessions: the client decrypts the logout answer of the server with the counter 7") {
    PacketDecryptor decryptor(kServerToClientDecryptKey);
    PacketEncryptor server(kServerToClientEncryptKey);
    Advance(decryptor, server, 7);
    CHECK(Decrypt(decryptor, Bytes({0xC3, 0x0D, 0xAF, 0x8B, 0x11, 0xD9, 0x9D, 0xDD, 0x03, 0xBE, 0x9C, 0x35,
                                    0x0C})) == Bytes({0xC3, 0x05, 0xF1, 0x02, 0x00}));
}

TEST_CASE(
    "[0001-CA-02] the largest plain C3 has 185 bytes, which become 255 on the wire; one more is refused") {
    std::vector<std::byte> largest(185, std::byte{0x11});
    largest[0] = std::byte{0xC3};
    largest[1] = std::byte{185};
    PacketEncryptor encryptor(kServerToClientEncryptKey);
    const auto wire = Encrypt(encryptor, largest);
    CHECK(wire.size() == 255);
    PacketDecryptor decryptor(kServerToClientDecryptKey);
    CHECK(Decrypt(decryptor, wire) == largest);

    std::vector<std::byte> tooLarge(186, std::byte{0x11});
    tooLarge[0] = std::byte{0xC3};
    tooLarge[1] = std::byte{186};
    std::vector<std::byte> out(400);
    CHECK(encryptor.Encrypt(tooLarge, out).error().message ==
          "a C3 of 186 bytes does not fit on the wire: the most is 185");
}

TEST_CASE("[0001-CA-02] a broken block or a size outside the blocks of 11 closes the connection") {
    PacketEncryptor server(kServerToClientEncryptKey);
    const auto wire = Encrypt(server, kServerMessage);

    // The size byte of a block is the real size xor a constant xor the checksum (section 4.2), so flipping
    // the same bit in the size byte and in the checksum keeps the size at 8: only the checksum is wrong.
    auto wrongChecksum = wire;
    wrongChecksum[11] = wrongChecksum[11] ^ std::byte{0x01}; // the size of the first block
    wrongChecksum[12] = wrongChecksum[12] ^ std::byte{0x01}; // the checksum of the first block
    std::vector<std::byte> out(wire.size());
    PacketDecryptor first(kServerToClientDecryptKey);
    const auto checksumRefused = first.Decrypt(wrongChecksum, out);
    REQUIRE_FALSE(checksumRefused.has_value());
    CHECK(checksumRefused.error().message == "the checksum of a block is wrong");

    // A byte of data changed on the way: the block opens into other bytes, with another checksum.
    auto changedData = wire;
    changedData[2] = changedData[2] ^ std::byte{0x01}; // the first byte of the first block
    PacketDecryptor changed(kServerToClientDecryptKey);
    const auto changedRefused = changed.Decrypt(changedData, out);
    REQUIRE_FALSE(changedRefused.has_value());
    CHECK(changedRefused.error().message == "the checksum of a block is wrong");

    auto tooManyBytes = wire;
    tooManyBytes[11] = tooManyBytes[11] ^ std::byte{0x01}; // the size of the first block: 8 becomes 9
    PacketDecryptor second(kServerToClientDecryptKey);
    CHECK(second.Decrypt(tooManyBytes, out).error().message.find("more than 8") != std::string::npos);

    auto cut = wire;
    cut.pop_back();
    cut[1] = std::byte{static_cast<std::uint8_t>(cut.size())};
    PacketDecryptor third(kServerToClientDecryptKey);
    CHECK(third.Decrypt(cut, out).error().message ==
          "an encrypted message of 23 bytes is not made of blocks of 11");
}
