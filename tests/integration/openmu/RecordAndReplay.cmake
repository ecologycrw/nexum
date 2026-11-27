# Records 20 seconds of a session on the OpenMU test server of compose.yaml and replays the recording without the
# server (specs/0002-gravacao-e-replay/spec.md, CA-12). The address of its Connect Server comes from
# NEXUM_OPENMU_ADDRESS, read when the test runs; without it, the script only says so, and CTest reports the test as
# skipped (TD-0034). Each run is checked by the script of the other MuClient tests
# (Games/MuOnline/App/CheckClientRun.cmake), and then:
#
# - the replay checks as many messages, ticks and checkpoints as the recording has, and every one matches;
# - the replay writes the same lines of the world as the session: the same objects come into view and leave it, in
#   the same order, and the same number of objects is in view at the end;
# - the recording has neither the account nor its password. The password of a test account is its name, which is
#   also the start of the name of its character, so the check looks for the name outside the name of the character.
#
# cmake -DCLIENT_RUN_CHECK=<CheckClientRun.cmake> -DPROGRAM=<path> -DINPUT=<file with the password of test0>
#       -DRECORDING=<path of the .mucap> -P RecordAndReplay.cmake

set(address "$ENV{NEXUM_OPENMU_ADDRESS}")
if(address STREQUAL "")
    message(STATUS "skipped: NEXUM_OPENMU_ADDRESS does not name the Connect Server of an OpenMU test server")
    return()
endif()

set(account test0)
set(character test0Dk)

# The lines of the world in an output, in order, with their level. The time lines and the session lines differ
# between a session and its replay; the lines of the world come from the events, so they do not.
function(world_lines output result)
    string(REPLACE "\r" "" text "${output}")
    string(REGEX MATCHALL "\\[[a-z]+\\] world: [^\n]*" lines "${text}")
    set(${result} "${lines}" PARENT_SCOPE)
endfunction()

# The session. A file left by an earlier run never passes for this one.
file(REMOVE "${RECORDING}")
set(ARGUMENTS --connect "${address}" --account ${account} --world-seconds 20 --record "${RECORDING}"
    --server-version "OpenMU 0.10.1")
set(EXIT_CODE 0)
set(PATTERNS
    "session: in the world"
    " (came into view|appeared) at "
    "client: stayed in the world as long as --world-seconds asked"
    "client: the session ends: 20 s in the world, pings sent: [0-9]+, objects in view: [0-9]+"
    "client: wrote the recording to [^\n]*: [1-9][0-9]* messages?, [1-9][0-9]* ticks? and [1-9][0-9]* checkpoints?")
set(ABSENT "[error]")
include("${CLIENT_RUN_CHECK}")
set(recorded_output "${output}")
string(REGEX MATCH "client: wrote the recording to [^\n]*: ([0-9]+ messages?, [0-9]+ ticks? and [0-9]+ checkpoints?)"
    counts_line "${recorded_output}")
set(counts "${CMAKE_MATCH_1}")
string(REGEX MATCH "objects in view: ([0-9]+)" objects_line "${recorded_output}")
set(objects "${CMAKE_MATCH_1}")

# The account and the password: every text of the file that has the name of the account is the name of the
# character.
file(STRINGS "${RECORDING}" texts REGEX "${account}")
string(REPLACE "${character}" "" texts_without_character "${texts}")
string(FIND "${texts_without_character}" "${account}" found_at)
if(NOT found_at EQUAL -1)
    message(FATAL_ERROR "the recording ${RECORDING} has the name of the account ${account} outside the name of "
        "the character ${character}: ${texts}")
endif()

# The replay, at the recorded pace, without the server.
set(ARGUMENTS --replay "${RECORDING}")
set(PATTERNS
    "replay: replaying ${counts}, recorded by the Nexum [^ ]+ with the server OpenMU 0\\.10\\.1"
    "replay: the replay reached the end of the recording, where the client ended it: every tick and checkpoint matched"
    "client: the session ends: [0-9]+ s in the world, the replay checked ${counts}, objects in view: ${objects}([^0-9]|$)")
set(ABSENT "[error]")
include("${CLIENT_RUN_CHECK}")
set(replayed_output "${output}")

world_lines("${recorded_output}" recorded_lines)
world_lines("${replayed_output}" replayed_lines)
if(NOT recorded_lines STREQUAL replayed_lines)
    list(JOIN recorded_lines "\n" recorded_text)
    list(JOIN replayed_lines "\n" replayed_text)
    message(FATAL_ERROR "the replay of ${RECORDING} wrote other lines of the world than the session.\n"
        "The session:\n${recorded_text}\nThe replay:\n${replayed_text}")
endif()
list(LENGTH recorded_lines line_count)
message(STATUS "the replay checked ${counts} and wrote the same ${line_count} lines of the world as the session")
