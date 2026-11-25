add_subdirectory(OtherGame)
add_subdirectory(frame_allocations)

if(NEXUM_SANITIZE)
    add_subdirectory(sanitizer_canary)
endif()
