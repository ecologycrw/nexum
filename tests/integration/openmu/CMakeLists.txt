# CA-14 of specs/0001-rede-ate-o-mundo/spec.md: MuClient against a real OpenMU without modifications, the test
# server of compose.yaml. The tests run only when NEXUM_OPENMU_ADDRESS names its Connect Server; without it, as in
# the CI, CTest reports them as skipped (TD-0034). With the server up, the label "openmu" runs only them, in about
# 4 minutes:
#
#   docker compose -f tests/integration/openmu/compose.yaml up -d
#   $env:NEXUM_OPENMU_ADDRESS = "127.0.0.1:44406"
#   ctest --preset windows-dev -L openmu
#
# The accounts are the public test accounts of the -demo mode: the password of each one is its name. MuClient reads
# the password from a file of test values, as through a pipe, like the other MuClient tests.
set(openmu_test_dir "${CMAKE_CURRENT_BINARY_DIR}/openmu_test")
file(MAKE_DIRECTORY "${openmu_test_dir}")
file(CONFIGURE OUTPUT "${openmu_test_dir}/test0-password.txt" CONTENT "test0\n" NEWLINE_STYLE LF)
set(wrong_password "not-the-password")
file(CONFIGURE OUTPUT "${openmu_test_dir}/wrong-password.txt" CONTENT "${wrong_password}\n" NEWLINE_STYLE LF)

# openmu_add_session_test(name arguments exit_code patterns input [ABSENT texts...])
#
# Runs MuClient with --connect to the test server, the arguments and the password in the input file, and checks the
# exit code, every pattern of the output and the texts it must not have. The lists are joined with $<SEMICOLON>, so
# each one reaches the script as a single argument. The tests take the server one at a time (RESOURCE_LOCK).
function(openmu_add_session_test name arguments exit_code patterns input)
    cmake_parse_arguments(PARSE_ARGV 5 run "" "" "ABSENT")
    list(JOIN arguments "$<SEMICOLON>" joined_arguments)
    list(JOIN patterns "$<SEMICOLON>" joined_patterns)
    list(JOIN run_ABSENT "$<SEMICOLON>" joined_absent)
    add_test(NAME ${name}
        COMMAND ${CMAKE_TEST_LAUNCHER} "${CMAKE_COMMAND}"
            "-DCLIENT_RUN_CHECK=${PROJECT_SOURCE_DIR}/Games/MuOnline/App/CheckClientRun.cmake"
            -DPROGRAM=$<TARGET_FILE:MuClient>
            "-DARGUMENTS=${joined_arguments}"
            -DEXIT_CODE=${exit_code}
            "-DPATTERNS=${joined_patterns}"
            "-DINPUT=${input}"
            "-DABSENT=${joined_absent}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckOpenMuSession.cmake")
    set_tests_properties(${name} PROPERTIES
        LABELS openmu
        SKIP_REGULAR_EXPRESSION "skipped: NEXUM_OPENMU_ADDRESS"
        RESOURCE_LOCK openmu)
endfunction()

# Three minutes in the world, in the order of section 6 of the protocol spec: the list of servers, the Game Server,
# the login, the character, the entry in the world and the objects in view. The ping goes every 20 seconds from the
# entry in the Game Server (section 8 and the finding of the real sessions, section 4.1), so 180 seconds in the
# world send 9. No line of the log is an error.
set(three_minutes_patterns
    "session: the Connect Server lists [1-9][0-9]* servers"
    "session: connecting to the Game Server at "
    "session: entered the Game Server, version 20404"
    "session: the login was accepted"
    "session: choosing the character "
    "session: in the world"
    "world: the hero 0x0200 is on the map "
    " came into view at "
    "client: stayed in the world as long as --world-seconds asked"
    "client: the session ends: 180 s in the world, pings sent: 9, objects in view: [1-9][0-9]*")
openmu_add_session_test(openmu_muclient_stays_three_minutes_in_the_world "--account;test0;--world-seconds;180" 0
    "${three_minutes_patterns}" "${openmu_test_dir}/test0-password.txt"
    ABSENT "[error]")

# A wrong password: the server refuses the login with the code of section 6.7, and the session ends with its
# meaning, never in the world. The password never reaches the output (CA-12).
openmu_add_session_test(openmu_muclient_reports_a_wrong_password "--account;test1" 1
    "session: the login was refused: invalid password;client: the session ends: never in the world"
    "${openmu_test_dir}/wrong-password.txt"
    ABSENT "${wrong_password}")
