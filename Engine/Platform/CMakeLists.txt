# The only module that talks to SDL and to the operating system, together with Engine/Render (INV-10).
# SDL stays private: the other modules see only the Public folder.
nexum_add_library(nexum_platform
    ALIAS Nexum::Platform
    SOURCES
        Private/PlatformInfo.cpp
        Private/VideoSystem.cpp
        Private/Window.cpp
    PUBLIC_DEPS Nexum::Core
    PRIVATE_DEPS SDL3::SDL3)

add_subdirectory(Tests)
