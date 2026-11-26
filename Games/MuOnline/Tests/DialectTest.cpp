// The 106.3 dialect: one test for each row of the table of variants of the protocol spec of the knowledge
// center (docs/specs/protocolo/protocolo-106-3-m2.md, section 5.2), and the identity the client sends in the
// login (section 5.1). CA-03 and CA-12.
#include <cstdint>
#include <string_view>

#include <doctest/doctest.h>

#include "Mu/Protocol/Dialect.hpp"

namespace {

using mu::protocol::HeaderKind;
namespace dialect = mu::protocol::dialect;

constexpr bool kNoSubCode = false;

// The first byte, the code, the subcode (when there is one) and the name of the variant the row names.
template <typename Packet>
void CheckVariant(HeaderKind header, std::uint8_t code, bool hasSubCode, std::uint8_t subCode,
                  std::string_view variant) {
    CHECK(Packet::kShape.name == variant);
    CHECK(Packet::kShape.header == header);
    CHECK(Packet::kShape.code == code);
    CHECK(Packet::kShape.hasSubCode == hasSubCode);
    if (hasSubCode) {
        CHECK(Packet::kShape.subCode == subCode);
    }
}

} // namespace

TEST_CASE("[0001-CA-03] the dialect receives the Hello C1 00 01 of the Connect Server") {
    CheckVariant<dialect::Hello>(HeaderKind::C1, 0x00, true, 0x01, "Hello");
}

TEST_CASE("[0001-CA-03] the dialect asks for the server list with C1 F4 06 and reads the current C2 F4 06") {
    CheckVariant<dialect::ServerListRequest>(HeaderKind::C1, 0xF4, true, 0x06, "ServerListRequest");
    CheckVariant<dialect::ServerListResponse>(HeaderKind::C2, 0xF4, true, 0x06, "ServerListResponse");
    // The current variant: the count and the id in 2 bytes each, so an entry starts at byte 7 (section 6.3).
    CHECK(dialect::ServerListResponse::kShape.minimumLength == 7);
}

TEST_CASE(
    "[0001-CA-03] the dialect asks for the address with the 6-byte C1 F4 03 and reads the ConnectionInfo") {
    CheckVariant<dialect::ConnectionInfoRequest>(HeaderKind::C1, 0xF4, true, 0x03, "ConnectionInfoRequest");
    CHECK(dialect::ConnectionInfoRequest::kDeclaredLength == 6);
    CheckVariant<dialect::ConnectionInfo>(HeaderKind::C1, 0xF4, true, 0x03, "ConnectionInfo");
    CHECK(dialect::ConnectionInfo::kDeclaredLength == 22);
}

TEST_CASE("[0001-CA-03] the dialect reads the GameServerEntered C1 F1 00") {
    CheckVariant<dialect::GameServerEntered>(HeaderKind::C1, 0xF1, true, 0x00, "GameServerEntered");
    CHECK(dialect::GameServerEntered::kDeclaredLength == 12);
}

TEST_CASE("[0001-CA-03] the dialect logs in with the LoginLongPassword C3 F1 01 of 60 bytes") {
    CheckVariant<dialect::Login>(HeaderKind::C3, 0xF1, true, 0x01, "LoginLongPassword");
    CHECK(dialect::Login::kDeclaredLength == 60);
}

TEST_CASE("[0001-CA-03] the dialect reads the LoginResponse C1 F1 01") {
    CheckVariant<dialect::LoginResponse>(HeaderKind::C1, 0xF1, true, 0x01, "LoginResponse");
}

TEST_CASE(
    "[0001-CA-03] the dialect asks for the characters with C1 F3 00 and reads the CharacterListExtended") {
    CheckVariant<dialect::RequestCharacterList>(HeaderKind::C1, 0xF3, true, 0x00, "RequestCharacterList");
    CheckVariant<dialect::CharacterList>(HeaderKind::C1, 0xF3, true, 0x00, "CharacterListExtended");
}

TEST_CASE("[0001-CA-03] the dialect reads the CharacterClassCreationUnlock C1 DE 00") {
    CheckVariant<dialect::CharacterClassCreationUnlock>(HeaderKind::C1, 0xDE, true, 0x00,
                                                        "CharacterClassCreationUnlock");
}

TEST_CASE("[0001-CA-03] the dialect selects the character with C1 F3 03") {
    CheckVariant<dialect::SelectCharacter>(HeaderKind::C1, 0xF3, true, 0x03, "SelectCharacter");
    CHECK(dialect::SelectCharacter::kDeclaredLength == 14);
}

TEST_CASE("[0001-CA-03] the dialect reads the CharacterInformationExtended C3 F3 03 of 93 bytes") {
    CheckVariant<dialect::CharacterInformation>(HeaderKind::C3, 0xF3, true, 0x03,
                                                "CharacterInformationExtended");
    CHECK(dialect::CharacterInformation::kDeclaredLength == 93);
}

TEST_CASE("[0001-CA-03] the dialect reads the MasterStatsUpdateExtended C1 F3 50") {
    CheckVariant<dialect::MasterStatsUpdate>(HeaderKind::C1, 0xF3, true, 0x50, "MasterStatsUpdateExtended");
}

TEST_CASE("[0001-CA-03] the dialect reads the CharacterInventory C4 F3 10") {
    CheckVariant<dialect::CharacterInventory>(HeaderKind::C4, 0xF3, true, 0x10, "CharacterInventory");
}

TEST_CASE("[0001-CA-03] the dialect reads the SkillListUpdate C1 F3 11 of version 3.0") {
    CheckVariant<dialect::SkillListUpdate>(HeaderKind::C1, 0xF3, true, 0x11, "SkillListUpdate");
}

TEST_CASE("[0001-CA-03] the dialect sends the Ping C3 0E 00 of 12 bytes") {
    CheckVariant<dialect::Ping>(HeaderKind::C3, 0x0E, true, 0x00, "Ping");
    CHECK(dialect::Ping::kDeclaredLength == 12);
}

TEST_CASE("[0001-CA-03] the dialect walks with C1 D4 and reads the ObjectWalkedExtended C1 D4") {
    CheckVariant<dialect::WalkRequest>(HeaderKind::C1, 0xD4, kNoSubCode, 0, "WalkRequest");
    CheckVariant<dialect::ObjectWalked>(HeaderKind::C1, 0xD4, kNoSubCode, 0, "ObjectWalkedExtended");
}

TEST_CASE("[0001-CA-03] the dialect reads the instant move ObjectMoved C1 15") {
    CheckVariant<dialect::ObjectMoved>(HeaderKind::C1, 0x15, kNoSubCode, 0, "ObjectMoved");
}

TEST_CASE("[0001-CA-03] the dialect reads the AddCharacterToScopeExtended C2 12") {
    CheckVariant<dialect::AddCharacterToScope>(HeaderKind::C2, 0x12, kNoSubCode, 0,
                                               "AddCharacterToScopeExtended");
}

TEST_CASE("[0001-CA-03] the dialect reads the AddNpcsToScope C2 13 of version 5.0, with 10 bytes per NPC") {
    CheckVariant<dialect::AddNpcsToScope>(HeaderKind::C2, 0x13, kNoSubCode, 0, "AddNpcsToScope");
    CHECK(dialect::AddNpcsToScope::NpcData::kSize == 10);
}

TEST_CASE("[0001-CA-03] the dialect reads the MapObjectOutOfScope C1 14") {
    CheckVariant<dialect::MapObjectOutOfScope>(HeaderKind::C1, 0x14, kNoSubCode, 0, "MapObjectOutOfScope");
}

TEST_CASE("[0001-CA-03] the dialect reads the ChangeTerrainAttributes C1 46") {
    CheckVariant<dialect::ChangeTerrainAttributes>(HeaderKind::C1, 0x46, kNoSubCode, 0,
                                                   "ChangeTerrainAttributes");
}

TEST_CASE("[0001-CA-03] the dialect reads the CurrentStatsExtended C1 26 FF") {
    CheckVariant<dialect::CurrentStats>(HeaderKind::C1, 0x26, true, 0xFF, "CurrentStatsExtended");
}

TEST_CASE("[0001-CA-12] the client introduces itself with the version 20404 and its own serial of 16 bytes") {
    CHECK(mu::protocol::kClientIdentity.version == "20404");
    CHECK(mu::protocol::kClientIdentity.serial == "NEXUM-CLIENT-001");
    CHECK(mu::protocol::kClientIdentity.serial.size() == 16);
    CHECK(mu::protocol::kConnectServerPort == 44406);
}
