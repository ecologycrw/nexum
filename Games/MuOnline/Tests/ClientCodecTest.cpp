// The transformation of bytes of the client on each connection: the framing alone on the Connect Server, and
// on the Game Server Xor32 on everything that goes out, then SimpleModulus with the counter on C3 and C4
// (docs/specs/protocolo/protocolo-106-3-m2.md, sections 2 and 4.1). The vectors are those of the finding
// 2026-11-21-cifra-simplemodulus-xor32-xor3.md (V3 and V5) and of the real sessions of the finding
// 2026-11-21-capturas-reais-sessao-106-3.md (the ping with the counter 1).
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/Cipher.hpp"
#include "Mu/Protocol/ClientCodec.hpp"

namespace {

using mu::protocol::ClientCodec;
using mu::protocol::Encryption;
using Bytes = std::vector<std::byte>;

[[nodiscard]] Bytes Make(std::initializer_list<std::uint8_t> values) {
    Bytes bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

class CollectingSink final : public nexum::net::MessageSink {
public:
    void Deliver(std::span<const std::byte> message) override {
        messages.emplace_back(message.begin(), message.end());
    }

    std::vector<Bytes> messages;
};

[[nodiscard]] Bytes Send(ClientCodec& codec, const Bytes& message) {
    Bytes wire(codec.WireCapacity(message.size()));
    auto written = codec.Send(message, wire);
    REQUIRE(written.has_value());
    wire.resize(*written);
    return wire;
}

} // namespace

TEST_CASE("[0001-CA-02] on the Connect Server the codec only frames, and sends the messages as they are") {
    ClientCodec codec(Encryption::None);
    CollectingSink sink;
    const Bytes pieces = Make({0xC1, 0x04, 0x00, 0x01, 0xC1, 0x04, 0xF4});
    REQUIRE(codec.Receive(pieces, sink).has_value());
    REQUIRE(codec.Receive(Make({0x06}), sink).has_value());
    REQUIRE(sink.messages.size() == 2);
    CHECK(sink.messages[0] == Make({0xC1, 0x04, 0x00, 0x01}));
    CHECK(sink.messages[1] == Make({0xC1, 0x04, 0xF4, 0x06}));

    CHECK(Send(codec, Make({0xC1, 0x06, 0xF4, 0x03, 0x00, 0x00})) ==
          Make({0xC1, 0x06, 0xF4, 0x03, 0x00, 0x00}));
}

TEST_CASE("[0001-CA-02] on the Game Server a C1 goes out with Xor32 only, and does not use the counter") {
    ClientCodec codec(Encryption::GameServer);
    // V3: from the byte after the code, each byte with the one before it and the key.
    CHECK(Send(codec, Make({0xC1, 0x04, 0xF3, 0x00})) == Make({0xC1, 0x04, 0xF3, 0x0D}));

    // The first C3 takes the counter 0, so the ping of the real session, with the counter 1, comes next.
    static_cast<void>(Send(codec, Make({0xC3, 0x05, 0xF1, 0x02, 0x00})));
    const Bytes ping = Make({0xC3, 0x0C, 0x0E, 0x00, 0x8D, 0x83, 0xB5, 0x08, 0x00, 0x00, 0x00, 0x00});
    CHECK(Send(codec, ping) ==
          Make({0xC3, 0x18, 0xEA, 0xD5, 0x0A, 0x20, 0x87, 0x25, 0x92, 0x78, 0xE4, 0xED,
                0xD8, 0xC0, 0x2C, 0x67, 0x3B, 0x6D, 0xE2, 0x41, 0xF4, 0x38, 0xED, 0xD3}));
}

TEST_CASE("[0001-CA-02] on the Game Server the codec decrypts the C3 of the server, even byte by byte") {
    ClientCodec codec(Encryption::GameServer);
    CollectingSink sink;
    // V5: the server's C3 with the counter 0, and a C1, which comes in the clear.
    const Bytes wire = Make({0xC3, 0x0D, 0x68, 0xCE, 0x2F, 0xC3, 0xCB, 0xD2, 0xE0, 0x8C, 0x81, 0x36, 0x08,
                             0xC1, 0x05, 0xF1, 0x01, 0x01});
    for (const std::byte value : wire) {
        REQUIRE(codec.Receive(std::span<const std::byte>(&value, 1), sink).has_value());
    }
    REQUIRE(sink.messages.size() == 2);
    CHECK(sink.messages[0] == Make({0xC3, 0x04, 0xF1, 0x01}));
    CHECK(sink.messages[1] == Make({0xC1, 0x05, 0xF1, 0x01, 0x01}));
}

TEST_CASE("[0001-CA-02] a wrong counter on the Game Server is an error, which closes the connection") {
    ClientCodec codec(Encryption::GameServer);
    CollectingSink sink;
    const Bytes wire = Make({0xC3, 0x0D, 0x68, 0xCE, 0x2F, 0xC3, 0xCB, 0xD2, 0xE0, 0x8C, 0x81, 0x36, 0x08});
    REQUIRE(codec.Receive(wire, sink).has_value());
    // The same message again: the counter 0 where the client expects 1 (V9).
    CHECK_FALSE(codec.Receive(wire, sink).has_value());
    CHECK(sink.messages.size() == 1);
}

TEST_CASE("the room the codec asks for always holds the message on the wire") {
    for (std::size_t size = 4; size <= 300; ++size) {
        CAPTURE(size);
        ClientCodec codec(Encryption::GameServer);
        CHECK(codec.WireCapacity(size) >= size);
        CHECK(codec.WireCapacity(size) >= mu::protocol::EncryptedLength(2, size));
        CHECK(codec.WireCapacity(size) >= mu::protocol::EncryptedLength(3, size));
    }
}
