#pragma once

// The 106.3 dialect (Mu/Protocol/Dialect.hpp), on the Connect Server, in the clear: the Hello, the list of
// servers and the address of the Game Server (sections 5.2 and 6.2 to 6.4 of the protocol spec).

#include "Mu/Protocol/Packets/ConnectServer/ConnectionInfo.hpp"
#include "Mu/Protocol/Packets/ConnectServer/ConnectionInfoRequest.hpp"
#include "Mu/Protocol/Packets/ConnectServer/Hello.hpp"
#include "Mu/Protocol/Packets/ConnectServer/ServerListRequest.hpp"
#include "Mu/Protocol/Packets/ConnectServer/ServerListResponse.hpp"

namespace mu::protocol::dialect {

using Hello = connect::Hello;
using ServerListRequest = connect::ServerListRequest;
using ServerListResponse = connect::ServerListResponse;
using ConnectionInfoRequest = connect::ConnectionInfoRequest;
using ConnectionInfo = connect::ConnectionInfo;

} // namespace mu::protocol::dialect
