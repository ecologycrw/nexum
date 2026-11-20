# The MU module: everything that belongs to MU and to the OpenMU server (docs/adr/0002-tres-camadas.md).
# The areas of the spec (Protocol, Session, Translate, Rules...) become subfolders of Public/Mu and Private.
nexum_add_library(MuOnline
    SOURCES
        Private/ModuleIdentity.cpp
    PUBLIC_DEPS Nexum::Core Nexum::GridRpg)

add_subdirectory(Tests)
add_subdirectory(App)
