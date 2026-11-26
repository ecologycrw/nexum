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
        Private/Protocol/Cipher.cpp
        Private/Protocol/ExtendedAppearance.cpp
        Private/Protocol/ExtendedItem.cpp
        Private/Protocol/FrameReader.cpp
        Private/Protocol/LoginMessage.cpp
        Private/Protocol/PacketFields.cpp
    PUBLIC_DEPS Nexum::Core Nexum::Debug Nexum::Render Nexum::Time Nexum::GridRpg)

# The code that reads and writes every packet of OpenMU, generated from its XML files (third_party/openmu) at each
# build, into the build folder (docs/adr/0034-packetgen-em-python-no-build.md). The code includes the headers as
# Mu/Protocol/Packets/<File>.hpp. The enums that every file shares come from CommonEnums.xml.
set(openmu_packets "${CMAKE_SOURCE_DIR}/third_party/openmu")
set(generated_packets "${CMAKE_CURRENT_BINARY_DIR}/Generated")
set(packet_include "Mu/Protocol/Packets")
nexum_generate_packets(
    XML "${openmu_packets}/CommonEnums.xml"
    NAMESPACE mu::protocol::common
    OUTPUT_DIR "${generated_packets}"
    INCLUDE "${packet_include}/CommonEnums.hpp"
    HEADER_VARIABLE common_header)
set(generated_headers "${common_header}")
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
        INCLUDE "${packet_include}/${file}Packets.hpp"
        TESTS "Tests/${file}PacketsTest.cpp"
        COMMON "${openmu_packets}/CommonEnums.xml"
        COMMON_NAMESPACE mu::protocol::common
        COMMON_INCLUDE "${packet_include}/CommonEnums.hpp"
        PEERS ${peers}
        HEADER_VARIABLE header
        TESTS_VARIABLE tests)
    list(APPEND generated_headers "${header}")
    list(APPEND generated_tests "${tests}")
endforeach()
# The rules of a generated file belong to a target of this folder; the tests in Tests/ wait for the same target.
add_custom_target(muonline_packets DEPENDS ${generated_headers} ${generated_tests})
add_dependencies(MuOnline muonline_packets)
target_include_directories(MuOnline PUBLIC "${generated_packets}")

add_subdirectory(Tests)
add_subdirectory(App)
