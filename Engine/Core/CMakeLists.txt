nexum_add_library(nexum_core
    ALIAS Nexum::Core
    SOURCES
        Private/Version.cpp
    PUBLIC_DEPS tl::expected)

target_compile_definitions(nexum_core PRIVATE
    NEXUM_ENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NEXUM_ENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NEXUM_ENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH})

add_subdirectory(Tests)
