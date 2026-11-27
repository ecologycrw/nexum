# The MU module: everything that belongs to MU and to the OpenMU server (docs/adr/0002-tres-camadas.md).
# The areas of the spec (Protocol, Session, Translate, Rules...) become subfolders of Public/Mu and Private.
# Foundation holds the first screen of the client, which goes away with the viewer of M1 (docs/TECH_DEBT.md).
nexum_add_library(MuOnline
    SOURCES
        Private/App/ClientOptions.cpp
        Private/Foundation/FoundationScene.cpp
        Private/Foundation/FoundationScreen.cpp
        Private/Foundation/ProofScene.cpp
        Private/Foundation/TickReportText.cpp
        Private/ModuleIdentity.cpp
        Private/Online/LiveServer.cpp
        Private/Online/NetworkTransport.cpp
        Private/Online/OnlineClient.cpp
        Private/Online/ReplayServer.cpp
        Private/Protocol/CharacterInformation.cpp
        Private/Protocol/Cipher.cpp
        Private/Protocol/ClientCodec.cpp
        Private/Protocol/ExtendedAppearance.cpp
        Private/Protocol/ExtendedItem.cpp
        Private/Protocol/FrameReader.cpp
        Private/Protocol/LoginMessage.cpp
        Private/Protocol/PacketFields.cpp
        Private/Protocol/PacketLog.cpp
        Private/Recording/Recorder.cpp
        Private/Recording/Recording.cpp
        Private/Recording/StateDigest.cpp
        Private/Session/MessageQueue.cpp
        Private/Session/Ping.cpp
        Private/Session/Session.cpp
        Private/Translate/Translator.cpp
        Private/World/EventQueue.cpp
        Private/World/WorldEvent.cpp
        Private/World/WorldView.cpp
    PUBLIC_DEPS Nexum::Core Nexum::Debug Nexum::Net Nexum::Render Nexum::Time Nexum::GridRpg
    PRIVATE_DEPS Nexum::Platform)

# The code that reads and writes every packet of OpenMU, generated from its XML files (third_party/openmu) at each
# build, into the build folder (docs/adr/0034-packetgen-em-python-no-build.md). Each packet is a header of its own,
# Mu/Protocol/Packets/<File>/<Packet>.hpp, and the code includes only the packets it uses, through the dialect
# (Mu/Protocol/Dialect): no header includes every packet, so a file that reads a few packets never compiles the
# others (proposal P-0016). The enums that every file shares come from CommonEnums.xml.
set(openmu_packets "${CMAKE_SOURCE_DIR}/third_party/openmu")
set(generated_packets "${CMAKE_CURRENT_BINARY_DIR}/Generated")
set(packet_include "Mu/Protocol/Packets")
nexum_generate_packets(
    XML "${openmu_packets}/CommonEnums.xml"
    NAMESPACE mu::protocol::common
    OUTPUT_DIR "${generated_packets}"
    INCLUDE_DIR "${packet_include}/Common"
    HEADERS_VARIABLE common_headers)
set(generated_headers ${common_headers})
set(generated_tests "")
# Each file of packets: its name in third_party/openmu, its namespace and the files whose enum values it names.
foreach(packets IN ITEMS "ConnectServer;connect;" "ClientToServer;client;ServerToClient" "ServerToClient;server;ClientToServer"
                         "ChatServer;chat;")
    list(GET packets 0 file)
    list(GET packets 1 namespace)
    list(GET packets 2 peer)
    set(peers "")
    if(peer)
        set(peers "${openmu_packets}/${peer}Packets.xml")
    endif()
    nexum_generate_packets(
        XML "${openmu_packets}/${file}Packets.xml"
        NAMESPACE "mu::protocol::${namespace}"
        OUTPUT_DIR "${generated_packets}"
        INCLUDE_DIR "${packet_include}/${file}"
        TESTS_PREFIX "Tests/${file}PacketsTest"
        COMMON "${openmu_packets}/CommonEnums.xml"
        COMMON_NAMESPACE mu::protocol::common
        COMMON_INCLUDE "${packet_include}/Common/Types.hpp"
        PEERS ${peers}
        HEADERS_VARIABLE headers
        TESTS_VARIABLE tests)
    list(APPEND generated_headers ${headers})
    list(APPEND generated_tests ${tests})
endforeach()
# The rules of a generated file belong to a target of this folder; the tests in Tests/ wait for the same target.
add_custom_target(muonline_packets DEPENDS ${generated_headers} ${generated_tests})
add_dependencies(MuOnline muonline_packets)
target_include_directories(MuOnline PUBLIC "${generated_packets}")

add_subdirectory(Tests)
add_subdirectory(App)
