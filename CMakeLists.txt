cmake_minimum_required(VERSION 3.28)

project(Nexum VERSION 0.0.0 LANGUAGES CXX)

# The project does not use C++20 modules (docs/adr/0003-linguagem-build-e-dependencias.md).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# The authoring mode exists only in the development build, never in the player build (INV-19).
option(NEXUM_AUTHORING "Build the authoring mode (tools for developers), never shipped to players" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(NexumSanitizers)
include(NexumModule)

find_package(SDL3 CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)

enable_testing()

add_subdirectory(Engine/Core)
add_subdirectory(Engine/Platform)
add_subdirectory(Kit/GridRpg)
add_subdirectory(Games/MuOnline)
add_subdirectory(tests)
