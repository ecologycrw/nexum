cmake_minimum_required(VERSION 3.28)

# The authoring mode exists only in the development build, never in the player build (INV-19).
option(NEXUM_AUTHORING "Build the authoring mode (tools for developers), never shipped to players" ON)

# The Tracy profiler (Engine/Debug), for the development build only. It is off by default because, when it is on,
# every program opens a network port for the Tracy viewer. The options come before project(), because vcpkg
# installs Tracy during project() only when the "profiler" feature of vcpkg.json is asked for.
option(NEXUM_PROFILER "Build with the Tracy profiler (development build only)" OFF)
include("${CMAKE_CURRENT_SOURCE_DIR}/Engine/Debug/ProfilerOption.cmake")
nexum_check_profiler_option("${NEXUM_PROFILER}" "${NEXUM_AUTHORING}")
if(NEXUM_PROFILER)
    list(APPEND VCPKG_MANIFEST_FEATURES "profiler")
endif()

project(Nexum VERSION 0.0.0 LANGUAGES CXX)

# The project does not use C++20 modules (docs/adr/0003-linguagem-build-e-dependencias.md).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(NexumSanitizers)
include(NexumModule)

find_package(SDL3 CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)

enable_testing()

add_subdirectory(Engine/Core)
add_subdirectory(Engine/Time)
add_subdirectory(Engine/Debug)
add_subdirectory(Engine/Platform)
add_subdirectory(Engine/Render)
add_subdirectory(Kit/GridRpg)
add_subdirectory(Games/MuOnline)
add_subdirectory(tests)
