# The MU module: everything that belongs to MU and to the OpenMU server (docs/adr/0002-tres-camadas.md).
# The areas of the spec (Protocol, Session, Translate, Rules...) become subfolders of Public/Mu and Private.
# Foundation holds the first screen of the client, which goes away with the viewer of M1 (docs/TECH_DEBT.md).
nexum_add_library(MuOnline
    SOURCES
        Private/App/ClientOptions.cpp
        Private/Foundation/FoundationScene.cpp
        Private/Foundation/FoundationScreen.cpp
        Private/Foundation/TickReportText.cpp
        Private/ModuleIdentity.cpp
    PUBLIC_DEPS Nexum::Core Nexum::Debug Nexum::Render Nexum::Time Nexum::GridRpg)

add_subdirectory(Tests)
add_subdirectory(App)
