# The device and target tests use the real graphics card: a hardware one on the developer's computer and
# a software one on the CI machines (specs/0000-fundacao/plan.md).
nexum_add_tests(nexum_render_tests
    SOURCES
        GpuDriverOrderTest.cpp
        GpuDeviceTest.cpp
        OffscreenTargetTest.cpp
        WindowTargetTest.cpp
    DEPS Nexum::Render)
