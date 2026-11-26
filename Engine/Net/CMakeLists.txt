# The transport of the engine: TCP connections, each on a thread of its own, with an incoming and a single outgoing
# queue (docs/adr/0006-modelo-de-threads.md, docs/adr/0010-rede-adaptador-em-camadas.md). It only moves bytes; the
# game gives the codec that frames and encrypts them. It is the only module that uses Asio (INV-10,
# docs/adr/0033-dono-de-cada-biblioteca-externa.md), through its error codes, without exceptions.
nexum_add_library(nexum_net
    ALIAS Nexum::Net
    SOURCES
        Private/Connection.cpp
    PUBLIC_DEPS Nexum::Core
    PRIVATE_DEPS asio::asio)

# A local TCP server for the tests of the connections and of the games, which cannot open sockets themselves
# (only this module may use Asio). It is a library of its own, so no program of the player carries it.
nexum_add_library(nexum_net_test_server
    ALIAS Nexum::NetTestServer
    SOURCES
        Private/LoopbackServer.cpp
    PUBLIC_DEPS Nexum::Core
    PRIVATE_DEPS asio::asio)

# Asio needs to know the oldest Windows it runs on: Windows 10.
if(WIN32)
    target_compile_definitions(nexum_net PRIVATE _WIN32_WINNT=0x0A00)
    target_compile_definitions(nexum_net_test_server PRIVATE _WIN32_WINNT=0x0A00)
endif()

add_subdirectory(Tests)
