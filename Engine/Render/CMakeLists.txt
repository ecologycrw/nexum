# The only module that talks to the GPU, through SDL GPU (INV-10, docs/adr/0029-sdl3-e-sdl-gpu.md).
# SDL stays private: the other modules see only the Public folder.
nexum_add_library(nexum_render
    ALIAS Nexum::Render
    SOURCES
        Private/DrawList.cpp
        Private/DrawRecording.cpp
        Private/GpuBackend.cpp
        Private/GpuDevice.cpp
        Private/OffscreenTarget.cpp
        Private/TextureStore.cpp
        Private/WindowTarget.cpp
    PUBLIC_DEPS Nexum::Core Nexum::Platform
    PRIVATE_DEPS SDL3::SDL3)

add_subdirectory(Tests)
