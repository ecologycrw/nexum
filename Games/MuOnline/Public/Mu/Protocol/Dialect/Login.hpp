#pragma once

// The 106.3 dialect (Mu/Protocol/Dialect.hpp), on the Game Server: the entry, the login, the list of
// characters, the choice and the data of the character (sections 5.2 and 6.5 to 6.11 of the protocol spec).

#include "Mu/Protocol/Packets/ClientToServer/LoginLongPassword.hpp"
#include "Mu/Protocol/Packets/ClientToServer/RequestCharacterList.hpp"
#include "Mu/Protocol/Packets/ClientToServer/SelectCharacter.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CharacterClassCreationUnlock.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CharacterInformationExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CharacterListExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/GameServerEntered.hpp"
#include "Mu/Protocol/Packets/ServerToClient/LoginResponse.hpp"

namespace mu::protocol::dialect {

using GameServerEntered = server::GameServerEntered;
using Login = client::LoginLongPassword;
using LoginResponse = server::LoginResponse;
using RequestCharacterList = client::RequestCharacterList;
using CharacterList = server::CharacterListExtended;
using CharacterClassCreationUnlock = server::CharacterClassCreationUnlock;
using SelectCharacter = client::SelectCharacter;
using CharacterInformation = server::CharacterInformationExtended;

} // namespace mu::protocol::dialect
