# The profiler exists only in the development build (D-18 of the reference spec, INV-19). The root CMakeLists.txt
# runs this check before project(), so a player build with the profiler stops before vcpkg installs Tracy.
# Engine/Debug/Tests/CheckProfilerOption.cmake runs the same check as a test.

function(nexum_check_profiler_option profiler authoring)
    if(profiler AND NOT authoring)
        message(FATAL_ERROR
            "NEXUM_PROFILER=ON needs NEXUM_AUTHORING=ON: the player build never has the profiler (INV-19).")
    endif()
endfunction()
