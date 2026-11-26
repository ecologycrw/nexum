#pragma once

// The 106.3 dialect: which variant of each packet the Nexum sends and receives, and how it introduces itself
// to OpenMU. It is the only place of the code that knows which of the generated variants the 106.3 client
// uses, so a new OpenMU means new XML files and a new look at this table
// (docs/adr/0034-packetgen-em-python-no-build.md).
//
// Source: the protocol spec of the knowledge center, docs/specs/protocolo/protocolo-106-3-m2.md, section 5
// (the table of variants in 5.2 and the version in 5.1), and the finding
// 2026-11-21-variantes-por-versao-m2.md.

#include <array>
#include <cstdint>
#include <string_view>

#include "Mu/Protocol/PacketFields.hpp"
#include "Mu/Protocol/Packets/ClientToServerPackets.hpp"
#include "Mu/Protocol/Packets/ConnectServerPackets.hpp"
#include "Mu/Protocol/Packets/ServerToClientPackets.hpp"

namespace mu::protocol {

// How the Nexum introduces itself in the login (section 5.1 and 6.6): the server reads the version "20404" as
// the 106.3 client in English. An unknown version would make the server answer in another dialect without a
// word, so the version is fixed here. The serial is never checked by the server; this one is the Nexum's own.
struct ClientIdentity {
    std::string_view version;
    std::string_view serial;
};

inline constexpr ClientIdentity kClientIdentity{"20404", "NEXUM-CLIENT-001"};

// The Connect Server of the 106.3 client listens on this port in a new installation of OpenMU (section 2).
inline constexpr std::uint16_t kConnectServerPort = 44406;

// The variant of each packet in the 106.3 dialect (section 5.2). The codes that change with the language are
// the English ones: D4 to walk and 15 for the instant move (section 5.1).
namespace dialect {

// Connect Server, in the clear.
using Hello = connect::Hello;
using ServerListRequest = connect::ServerListRequest;
using ServerListResponse = connect::ServerListResponse;
using ConnectionInfoRequest = connect::ConnectionInfoRequest;
using ConnectionInfo = connect::ConnectionInfo;

// Game Server: the login and the choice of the character.
using GameServerEntered = server::GameServerEntered;
using Login = client::LoginLongPassword;
using LoginResponse = server::LoginResponse;
using RequestCharacterList = client::RequestCharacterList;
using CharacterList = server::CharacterListExtended;
using CharacterClassCreationUnlock = server::CharacterClassCreationUnlock;
using SelectCharacter = client::SelectCharacter;
using CharacterInformation = server::CharacterInformationExtended;
using MasterStatsUpdate = server::MasterStatsUpdateExtended;
using CharacterInventory = server::CharacterInventory;
using SkillListUpdate = server::SkillListUpdate;

// Game Server: the world.
using Ping = client::Ping;
using WalkRequest = client::WalkRequest;
using ObjectWalked = server::ObjectWalkedExtended;
using ObjectMoved = server::ObjectMoved;
using AddCharacterToScope = server::AddCharacterToScopeExtended;
using AddNpcsToScope = server::AddNpcsToScope;
using MapObjectOutOfScope = server::MapObjectOutOfScope;
using ChangeTerrainAttributes = server::ChangeTerrainAttributes;
using CurrentStats = server::CurrentStatsExtended;

} // namespace dialect

} // namespace mu::protocol
