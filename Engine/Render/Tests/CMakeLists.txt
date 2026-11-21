# The device, texture and target tests use the real graphics card: a hardware one on the developer's computer
# and a software one on the CI machines (specs/0000-fundacao/plan.md). The draw list test needs no GPU.
nexum_add_tests(nexum_render_tests
    SOURCES
        DrawListTest.cpp
        GpuDriverOrderTest.cpp
        GpuDeviceTest.cpp
        OffscreenTargetTest.cpp
        TextureCopyTest.cpp
        TextureStoreTest.cpp
        WindowTargetTest.cpp
    DEPS Nexum::Render)
