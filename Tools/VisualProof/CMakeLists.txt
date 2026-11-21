# The visual proof (docs/VISUAL.md): visualproof compares a capture with the image the owner approved, with the
# tolerance of the scene, and refuses an empty or single-color image. It is the seed of Tools/parity, which adds
# the perceptual comparison in M3 (specs/0000-fundacao/plan.md). Like every tool, it exists only in the
# development build (INV-02).
nexum_add_library(visualproof_comparison
    SOURCES
        Private/ImageComparison.cpp
        Private/ProofOptions.cpp
    PUBLIC_DEPS Nexum::Core
    PRIVATE_DEPS Nexum::Platform)

nexum_add_program(visualproof
    SOURCES Private/Main.cpp
    DEPS visualproof_comparison)

add_subdirectory(Tests)
