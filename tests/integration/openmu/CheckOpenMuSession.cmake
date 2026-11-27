# Runs MuClient against the OpenMU test server of compose.yaml (specs/0001-rede-ate-o-mundo/spec.md, CA-14). The
# address of its Connect Server comes from NEXUM_OPENMU_ADDRESS, read when the test runs. Without it, the script only
# says so, and CTest reports the test as skipped (SKIP_REGULAR_EXPRESSION in CMakeLists.txt). With it, the check is
# the one of the other MuClient tests (Games/MuOnline/App/CheckClientRun.cmake), with --connect in front.
#
# cmake -DCLIENT_RUN_CHECK=<CheckClientRun.cmake> -DPROGRAM=<path> -DARGUMENTS=<list> -DEXIT_CODE=<number>
#       -DPATTERNS=<list of regular expressions> -DINPUT=<file> [-DABSENT=<list of texts>] -P CheckOpenMuSession.cmake

set(address "$ENV{NEXUM_OPENMU_ADDRESS}")
if(address STREQUAL "")
    message(STATUS "skipped: NEXUM_OPENMU_ADDRESS does not name the Connect Server of an OpenMU test server")
else()
    list(PREPEND ARGUMENTS --connect "${address}")
    include("${CLIENT_RUN_CHECK}")
endif()
