# The only module that talks to SDL and to the operating system, together with Engine/Render (INV-10).
# SDL stays private: the other modules see only the Public folder.
nexum_add_library(nexum_platform
    ALIAS Nexum::Platform
    SOURCES
        Private/Application.cpp
        Private/FailureReports.cpp
        Private/PlatformInfo.cpp
        Private/Png.cpp
        Private/SdlLogRoute.cpp
        Private/Terminal.cpp
        Private/VideoSystem.cpp
        Private/Wait.cpp
        Private/Window.cpp
    PUBLIC_DEPS Nexum::Core Nexum::Debug
    PRIVATE_DEPS SDL3::SDL3)

# Application.cpp needs the declarations of SDL_main.h, but not the main function that the header would add:
# the main function is the program's own (Nexum/Platform/Application.hpp).
set_source_files_properties(Private/Application.cpp PROPERTIES COMPILE_DEFINITIONS SDL_MAIN_NOIMPL)

# FailureReports.cpp includes windows.h for SetErrorMode, and Terminal.cpp for the console: the smallest part of
# it, without the min and max macros, which would break std::min and std::max.
if(WIN32)
    set_source_files_properties(Private/FailureReports.cpp Private/Terminal.cpp PROPERTIES COMPILE_DEFINITIONS
        "WIN32_LEAN_AND_MEAN;NOMINMAX")
endif()

add_subdirectory(Tests)
