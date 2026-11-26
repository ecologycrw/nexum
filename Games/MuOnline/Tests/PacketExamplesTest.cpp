// The example packets of the knowledge center (CA-05), read with the 106.3 dialect and written again: the
// same fields give the same bytes. The bytes come from the real sessions of the finding
// 2026-11-21-capturas-reais-sessao-106-3.md (section 3), from the walks V1 to V6 of the protocol spec
// (docs/specs/protocolo/protocolo-106-3-m2.md, section 8.1) and from the examples of the plan of slice 1,
// computed from the spec (sections 6.3 to 6.10).
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

#include "Mu/Protocol/Dialect.hpp"
#include "Mu/Protocol/LoginMessage.hpp"

namespace {

namespace dialect = mu::protocol::dialect;

[[nodiscard]] std::vector<std::byte> Bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    for (const std::uint8_t value : values) {
        bytes.push_back(std::byte{value});
    }
    return bytes;
}

[[nodiscard]] std::vector<std::byte> Text(std::string_view text, std::size_t size) {
    std::vector<std::byte> bytes(size);
    for (std::size_t index = 0; index < text.size(); ++index) {
        bytes[index] = static_cast<std::byte>(text[index]);
    }
    return bytes;
}

[[nodiscard]] std::vector<std::byte> Join(std::initializer_list<std::vector<std::byte>> parts) {
    std::vector<std::byte> joined;
    for (const auto& part : parts) {
        joined.insert(joined.end(), part.begin(), part.end());
    }
    return joined;
}

// A message of this packet with the size of `bytes`, started by its Writer: the fixed bytes are already
// there.
template <typename Packet>
[[nodiscard]] typename Packet::Writer Start(std::vector<std::byte>& buffer, std::size_t size) {
    buffer.assign(size, std::byte{0});
    auto writer = Packet::Writer::Create(buffer);
    REQUIRE(writer.has_value());
    return *writer;
}

template <typename Packet>
[[nodiscard]] typename Packet::View Read(const std::vector<std::byte>& bytes) {
    auto view = Packet::View::Read(bytes);
    REQUIRE(view.has_value());
    return *view;
}

} // namespace

TEST_CASE("[0001-CA-05] the Hello and the request of the server list have only their fixed bytes") {
    const auto hello = Bytes({0xC1, 0x04, 0x00, 0x01});
    static_cast<void>(Read<dialect::Hello>(hello));
    std::vector<std::byte> written;
    static_cast<void>(Start<dialect::Hello>(written, hello.size()));
    CHECK(written == hello);

    const auto request = Bytes({0xC1, 0x04, 0xF4, 0x06});
    static_cast<void>(Read<dialect::ServerListRequest>(request));
    static_cast<void>(Start<dialect::ServerListRequest>(written, request.size()));
    CHECK(written == request);
}

TEST_CASE("[0001-CA-05] the server list of a real session has 3 servers, with the ids 0, 1 and 2") {
    const auto bytes = Bytes({0xC2, 0x00, 0x13, 0xF4, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
                              0x00, 0x00, 0x02, 0x00, 0x00, 0x00});
    const auto list = Read<dialect::ServerListResponse>(bytes);
    CHECK(list.ServerCount() == 3); // big-endian
    REQUIRE(list.ServersCount() == 3);
    for (std::uint16_t index = 0; index < 3; ++index) {
        const auto server = list.Servers(index);
        REQUIRE(server.has_value());
        CHECK(server->ServerId() == index); // little-endian
        CHECK(server->LoadPercentage() == 0);
    }

    std::vector<std::byte> written;
    const auto writer = Start<dialect::ServerListResponse>(written, bytes.size());
    writer.SetServerCount(3);
    for (std::uint16_t index = 0; index < 3; ++index) {
        writer.Servers(index)->SetServerId(index);
    }
    CHECK(written == bytes);
}

TEST_CASE("[0001-CA-05] the client asks for the address of the server 0 with C1 06 F4 03 00 00") {
    const auto bytes = Bytes({0xC1, 0x06, 0xF4, 0x03, 0x00, 0x00});
    CHECK(Read<dialect::ConnectionInfoRequest>(bytes).ServerId() == 0);
    std::vector<std::byte> written;
    Start<dialect::ConnectionInfoRequest>(written, bytes.size()).SetServerId(0);
    CHECK(written == bytes);
}

TEST_CASE("[0001-CA-05] the address of the Game Server is 127.0.0.1 and the port 45902, little-endian") {
    const auto bytes = Join({Bytes({0xC1, 0x16, 0xF4, 0x03}), Text("127.0.0.1", 16), Bytes({0x4E, 0xB3})});
    const auto info = Read<dialect::ConnectionInfo>(bytes);
    CHECK(info.IpAddress().Equals("127.0.0.1"));
    CHECK(info.Port() == 45902);

    std::vector<std::byte> written;
    const auto writer = Start<dialect::ConnectionInfo>(written, bytes.size());
    REQUIRE(writer.SetIpAddress("127.0.0.1").has_value());
    writer.SetPort(45902);
    CHECK(written == bytes);
}

TEST_CASE("[0001-CA-05] the GameServerEntered of a real session: success, the player id 0200 and the version "
          "20404") {
    const auto bytes = Bytes({0xC1, 0x0C, 0xF1, 0x00, 0x01, 0x02, 0x00, 0x32, 0x30, 0x34, 0x30, 0x34});
    const auto entered = Read<dialect::GameServerEntered>(bytes);
    CHECK(entered.Success());
    CHECK(entered.PlayerId() == 0x0200); // big-endian
    CHECK(entered.VersionString().Equals("20404"));

    std::vector<std::byte> written;
    const auto writer = Start<dialect::GameServerEntered>(written, bytes.size());
    writer.SetSuccess(true);
    writer.SetPlayerId(0x0200);
    REQUIRE(writer.SetVersionString("20404").has_value());
    CHECK(written == bytes);
}

TEST_CASE("[0001-CA-05] the LoginResponse says the password is invalid with 0 and accepts with 1") {
    using Result = dialect::LoginResponse::LoginResult;
    const auto refused = Bytes({0xC1, 0x05, 0xF1, 0x01, 0x00});
    CHECK(Read<dialect::LoginResponse>(refused).Success() == Result::InvalidPassword);
    const auto accepted = Bytes({0xC1, 0x05, 0xF1, 0x01, 0x01});
    CHECK(Read<dialect::LoginResponse>(accepted).Success() == Result::Okay);

    std::vector<std::byte> written;
    Start<dialect::LoginResponse>(written, refused.size()).SetSuccess(Result::InvalidPassword);
    CHECK(written == refused);
}

TEST_CASE("[0001-CA-05] the login comes out as in a real session, but with the Nexum's own serial") {
    // The plain login of the real session (section 6.6, counter 8): "test0" in both fields with Xor3, the
    // time 08 B7 5A CE big-endian, the version 20404, and then the serial, here NEXUM-CLIENT-001, and a spare
    // zero.
    const auto expected =
        Join({Bytes({0xC3, 0x3C, 0xF1, 0x01, 0x88, 0xAA, 0xD8, 0x88, 0xFF, 0xAB, 0xFC, 0xCF, 0xAB, 0xFC, 0x88,
                     0xAA, 0xD8, 0x88, 0xFF, 0xAB, 0xFC, 0xCF, 0xAB, 0xFC, 0xCF, 0xAB, 0xFC, 0xCF, 0xAB, 0xFC,
                     0xCF, 0xAB, 0xFC, 0xCF, 0x08, 0xB7, 0x5A, 0xCE, 0x32, 0x30, 0x34, 0x30, 0x34}),
              Text("NEXUM-CLIENT-001", 16), Bytes({0x00})});
    mu::protocol::LoginMessage login{};
    REQUIRE(mu::protocol::WriteLogin("test0", "test0", 146234062, login).has_value());
    CHECK(std::vector<std::byte>(login.begin(), login.end()) == expected);
    CHECK(Read<dialect::Login>(expected).TickCount() == 146234062);
}

TEST_CASE("[0001-CA-05] a user or a password longer than its field is refused before the login is built") {
    mu::protocol::LoginMessage login{};
    CHECK(mu::protocol::WriteLogin("elevenchars", "test0", 0, login).error().message ==
          "the user has 11 bytes; the most is 10");
    CHECK_FALSE(mu::protocol::WriteLogin("test0", "twenty-one-characters", 0, login).has_value());
    CHECK(mu::protocol::WriteLogin("tenletters", "twenty-characters-ok", 0, login).has_value());
}

TEST_CASE("[0001-CA-05] the request of the character list and the selection of test0Dk") {
    const auto request = Bytes({0xC1, 0x05, 0xF3, 0x00, 0x00});
    CHECK(Read<dialect::RequestCharacterList>(request).Language() == 0);
    std::vector<std::byte> written;
    Start<dialect::RequestCharacterList>(written, request.size()).SetLanguage(0);
    CHECK(written == request);

    const auto select = Join({Bytes({0xC1, 0x0E, 0xF3, 0x03}), Text("test0Dk", 10)});
    CHECK(Read<dialect::SelectCharacter>(select).Name().Equals("test0Dk"));
    REQUIRE(Start<dialect::SelectCharacter>(written, select.size()).SetName("test0Dk").has_value());
    CHECK(written == select);
}

TEST_CASE("[0001-CA-05] the ping of a real session: the time little-endian and the speeds 29 and 29") {
    const auto bytes = Bytes({0xC3, 0x0C, 0x0E, 0x00, 0x9C, 0xA6, 0xB7, 0x08, 0x1D, 0x00, 0x1D, 0x00});
    const auto ping = Read<dialect::Ping>(bytes);
    CHECK(ping.TickCount() == 0x08B7A69C);
    CHECK(ping.AttackSpeed() == 29);
    CHECK(ping.MagicSpeed() == 29);

    std::vector<std::byte> written;
    const auto writer = Start<dialect::Ping>(written, bytes.size());
    writer.SetTickCount(0x08B7A69C);
    writer.SetAttackSpeed(29);
    writer.SetMagicSpeed(29);
    CHECK(written == bytes);
}

TEST_CASE("[0001-CA-05] the start of the C2 12 of the player himself in a real session") {
    // The 26 bytes before the appearance: the id 0200 little-endian, at (145, 123), standing, the speeds 29
    // and 29 and the name "test0Dk". The appearance and the count of effects follow (54 bytes with no
    // effect).
    auto bytes = Bytes({0xC2, 0x00, 0x36, 0x12, 0x00, 0x02, 0x91, 0x7B, 0x91, 0x7B, 0x00, 0x00, 0x1D,
                        0x00, 0x1D, 0x00, 0x74, 0x65, 0x73, 0x74, 0x30, 0x44, 0x6B, 0x00, 0x00, 0x00});
    bytes.resize(54);
    const auto player = Read<dialect::AddCharacterToScope>(bytes);
    CHECK(player.Id() == 0x0200);
    CHECK(player.CurrentPositionX() == 145);
    CHECK(player.CurrentPositionY() == 123);
    CHECK(player.TargetPositionX() == 145);
    CHECK(player.TargetPositionY() == 123);
    CHECK(player.Rotation() == 0);
    CHECK(player.AttackSpeed() == 29);
    CHECK(player.MagicSpeed() == 29);
    CHECK(player.Name().Equals("test0Dk"));
    CHECK(player.AppearanceAndEffects().size() == 28);

    std::vector<std::byte> written;
    const auto writer = Start<dialect::AddCharacterToScope>(written, bytes.size());
    writer.SetId(0x0200);
    writer.SetCurrentPositionX(145);
    writer.SetCurrentPositionY(123);
    writer.SetTargetPositionX(145);
    writer.SetTargetPositionY(123);
    writer.SetAttackSpeed(29);
    writer.SetMagicSpeed(29);
    REQUIRE(writer.SetName("test0Dk").has_value());
    CHECK(written == bytes);
}

TEST_CASE(
    "[0001-CA-05] the walks V1 to V6 read their origin, steps and rotation, and are written again the same") {
    struct Walk {
        std::vector<std::byte> bytes;
        std::uint8_t x;
        std::uint8_t y;
        std::uint8_t steps;
        std::uint8_t rotation;
        std::vector<std::uint8_t> directions; // the column "Passos" of section 8.1
    };
    const std::array<Walk, 6> walks{{
        {Bytes({0xC1, 0x08, 0xD4, 0x93, 0x78, 0x44, 0x33, 0x44}), 147, 120, 4, 4, {3, 3, 4, 4}},
        {Bytes({0xC1, 0x08, 0xD4, 0x91, 0x7B, 0x53, 0x65, 0x50}), 145, 123, 3, 5, {6, 5, 5}},
        {Bytes({0xC1, 0x08, 0xD4, 0x8A, 0x74, 0x73, 0x70, 0x70}), 138, 116, 3, 7, {7, 0, 7}},
        {Bytes({0xC1, 0x08, 0xD4, 0x85, 0x68, 0x23, 0x11, 0x20}), 133, 104, 3, 2, {1, 1, 2}},
        {Bytes({0xC1, 0x06, 0xD4, 0xBB, 0x7E, 0x10}), 187, 126, 0, 1, {}},
        {Bytes({0xC1, 0x0D, 0xD4, 0x64, 0x64, 0x4D, 0x01, 0x23, 0x45, 0x67, 0x01, 0x23, 0x40}),
         100,
         100,
         13,
         4,
         {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4}},
    }};
    for (const auto& walk : walks) {
        CAPTURE(walk.x);
        const auto view = Read<dialect::WalkRequest>(walk.bytes);
        CHECK(view.SourceX() == walk.x);
        CHECK(view.SourceY() == walk.y);
        CHECK(view.StepCount() == walk.steps);         // the low nibble of byte 5
        CHECK(view.TargetRotation() == walk.rotation); // the high nibble
        // The first step in the high nibble of each byte of directions, the second in the low one.
        REQUIRE(view.Directions().size() == (walk.directions.size() + 1) / 2);
        for (std::size_t step = 0; step < walk.directions.size(); ++step) {
            const auto both = std::to_integer<std::uint8_t>(view.Directions()[step / 2]);
            const auto direction = static_cast<std::uint8_t>(step % 2 == 0 ? both >> 4U : both & 0x0FU);
            CHECK(direction == walk.directions[step]);
        }
        std::vector<std::byte> written;
        const auto writer = Start<dialect::WalkRequest>(written, walk.bytes.size());
        writer.SetSourceX(walk.x);
        writer.SetSourceY(walk.y);
        writer.SetStepCount(walk.steps);
        writer.SetTargetRotation(walk.rotation);
        REQUIRE(writer.SetDirections(view.Directions()).has_value());
        CHECK(written == walk.bytes);
    }
}

TEST_CASE("[0001-CA-05] the echo of the walk of a real session: the object 0200 goes from (145, 123) to "
          "(144, 126)") {
    const auto bytes = Bytes({0xC1, 0x0A, 0xD4, 0x02, 0x00, 0x91, 0x7B, 0x90, 0x7E, 0x30});
    const auto echo = Read<dialect::ObjectWalked>(bytes);
    CHECK(echo.ObjectId() == 0x0200); // big-endian
    CHECK(echo.SourceX() == 145);
    CHECK(echo.SourceY() == 123);
    CHECK(echo.TargetX() == 144);
    CHECK(echo.TargetY() == 126);
    CHECK(echo.TargetRotation() == 3); // the rotation from before the walk
    CHECK(echo.StepCount() == 0);

    std::vector<std::byte> written;
    const auto writer = Start<dialect::ObjectWalked>(written, bytes.size());
    writer.SetObjectId(0x0200);
    writer.SetSourceX(145);
    writer.SetSourceY(123);
    writer.SetTargetX(144);
    writer.SetTargetY(126);
    writer.SetTargetRotation(3);
    CHECK(written == bytes);
}
