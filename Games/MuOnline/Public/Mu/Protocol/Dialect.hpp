#pragma once

// The 106.3 dialect: which variant of each packet the Nexum sends and receives, and how it introduces itself
// to OpenMU. It is the only place of the code that knows which of the generated variants the 106.3 client
// uses, so a new OpenMU means new XML files and a new look at this table
// (docs/adr/0034-packetgen-em-python-no-build.md).
//
// The table of variants comes in parts, one for each step of the protocol spec, in Mu/Protocol/Dialect/:
// ConnectServer.hpp (sections 6.2 to 6.4), Login.hpp (6.5 to 6.11), FirstEntry.hpp (6.12) and World.hpp (8).
// Each file of the code includes only the parts it uses, and each part includes only its packets. This header
// holds only the identity of the client and includes no packet, so nothing brings every packet along
// (proposal P-0016).
//
// Source: the protocol spec of the knowledge center, docs/specs/protocolo/protocolo-106-3-m2.md, section 5
// (the table of variants in 5.2 and the version in 5.1), and the finding
// 2026-11-21-variantes-por-versao-m2.md.

#include <cstdint>
#include <string_view>

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

} // namespace mu::protocol
