# Runs the check of the profiler option (Engine/Debug/ProfilerOption.cmake) with the values given on the command
# line, the same check the root CMakeLists.txt runs before project():
#
#     cmake -DPROFILER=ON -DAUTHORING=OFF -P CheckProfilerOption.cmake
#
# A refused combination ends the script with the error of the check. An accepted one writes a line saying so.

include("${CMAKE_CURRENT_LIST_DIR}/../ProfilerOption.cmake")
nexum_check_profiler_option("${PROFILER}" "${AUTHORING}")
message(STATUS "The profiler option was accepted: NEXUM_PROFILER=${PROFILER}, NEXUM_AUTHORING=${AUTHORING}")
