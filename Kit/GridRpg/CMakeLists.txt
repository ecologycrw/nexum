# The kit of the genre: what every grid action RPG with an overhead camera and an authoritative server needs.
# It only knows the public interface of the engine, and nothing of a specific game (INV-02, INV-03).
nexum_add_library(nexum_gridrpg
    ALIAS Nexum::GridRpg
    SOURCES
        Private/KitIdentity.cpp
    PUBLIC_DEPS Nexum::Core)

add_subdirectory(Tests)
