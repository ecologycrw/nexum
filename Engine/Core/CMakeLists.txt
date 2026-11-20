nexum_add_library(nexum_core
    ALIAS Nexum::Core
    SOURCES
        Private/Assert.cpp
        Private/Version.cpp
    PUBLIC_DEPS tl::expected)

target_compile_definitions(nexum_core PRIVATE
    NEXUM_ENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NEXUM_ENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NEXUM_ENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH})

# NEXUM_ASSERT is on in the development build and off in the player build (Nexum/Core/Assert.hpp). The
# definition is public, so every target that uses the macro sees the same value.
target_compile_definitions(nexum_core PUBLIC NEXUM_ASSERTS_ENABLED=$<BOOL:${NEXUM_AUTHORING}>)

add_subdirectory(Tests)
