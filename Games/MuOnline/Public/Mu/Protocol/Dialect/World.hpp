#pragma once

// The 106.3 dialect (Mu/Protocol/Dialect.hpp), in the world: the ping, the walk, the objects that come into
// the view and leave it, the terrain and the stats of the hero (sections 5.2 and 8 of the protocol spec). The
// codes that change with the language are the English ones: D4 to walk and 15 for the instant move
// (section 5.1).

#include "Mu/Protocol/Packets/ClientToServer/Ping.hpp"
#include "Mu/Protocol/Packets/ClientToServer/WalkRequest.hpp"
#include "Mu/Protocol/Packets/ServerToClient/AddCharacterToScopeExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/AddNpcsToScope.hpp"
#include "Mu/Protocol/Packets/ServerToClient/ChangeTerrainAttributes.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CurrentStatsExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/MapObjectOutOfScope.hpp"
#include "Mu/Protocol/Packets/ServerToClient/ObjectMoved.hpp"
#include "Mu/Protocol/Packets/ServerToClient/ObjectWalkedExtended.hpp"

namespace mu::protocol::dialect {

using Ping = client::Ping;
using WalkRequest = client::WalkRequest;
using ObjectWalked = server::ObjectWalkedExtended;
using ObjectMoved = server::ObjectMoved;
using AddCharacterToScope = server::AddCharacterToScopeExtended;
using AddNpcsToScope = server::AddNpcsToScope;
using MapObjectOutOfScope = server::MapObjectOutOfScope;
using ChangeTerrainAttributes = server::ChangeTerrainAttributes;
using CurrentStats = server::CurrentStatsExtended;

} // namespace mu::protocol::dialect
