# The visual proof of the slices (docs/VISUAL.md). The approved images live in approved/, a "trava": only an image
# the owner approved goes there. MuClient --capture and visualproof exist only in the development build.
#
# visual_proof_add_scene(name approved_file tolerance) captures the proof scene and compares it with the approved
# image. The capture stays in the build folder, for the AI and the reviewer to open.
function(visual_proof_add_scene name approved tolerance)
    add_test(NAME ${name}
        COMMAND ${CMAKE_TEST_LAUNCHER} "${CMAKE_COMMAND}"
            -DCLIENT=$<TARGET_FILE:MuClient>
            -DVISUALPROOF=$<TARGET_FILE:visualproof>
            "-DCAPTURE=${CMAKE_CURRENT_BINARY_DIR}/${approved}"
            "-DAPPROVED=${CMAKE_CURRENT_SOURCE_DIR}/approved/${approved}"
            -DTOLERANCE=${tolerance}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckProofScene.cmake")
endfunction()

if(NEXUM_AUTHORING)
    # CA-07 of specs/0000-fundacao/spec.md: the test card at tick 50 with alpha 0.5, at most 1 level per channel
    # and no pixel beyond.
    visual_proof_add_scene(visual_proof_0000_test_card 0000-test-card.png 1)
endif()
