# The visual proof of a scene (docs/VISUAL.md): MuClient captures the proof scene without a window, and
# visualproof compares it with the image the owner approved, with the tolerance of the scene.
#
# cmake -DCLIENT=<MuClient> -DVISUALPROOF=<visualproof> -DCAPTURE=<new PNG file> -DAPPROVED=<approved PNG file>
#       -DTOLERANCE=<levels> -P CheckProofScene.cmake

# A capture left by an earlier run never stands in for a new one.
file(REMOVE "${CAPTURE}")
execute_process(COMMAND "${CLIENT}" --capture "${CAPTURE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output TIMEOUT 300)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "MuClient --capture ended with ${result} instead of 0:\n${output}")
endif()
message(STATUS "MuClient --capture ${CAPTURE}:\n${output}")

if(NOT EXISTS "${APPROVED}")
    message(FATAL_ERROR "there is no approved image at ${APPROVED}. The owner looks at the capture, ${CAPTURE}, "
        "and only an image the owner approved goes there (docs/VISUAL.md).")
endif()
execute_process(COMMAND "${VISUALPROOF}" "${CAPTURE}" "${APPROVED}" --tolerance "${TOLERANCE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE proof ERROR_VARIABLE proof TIMEOUT 120)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "the capture ${CAPTURE} is not the approved image ${APPROVED}:\n${proof}")
endif()
message(STATUS "${proof}")
