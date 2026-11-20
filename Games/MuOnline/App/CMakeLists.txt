# The entry point of the game. Today it only reports its version and the build kind.
nexum_add_program(MuClient
    SOURCES Private/Main.cpp
    DEPS MuOnline)

# The player build never carries the authoring mode (INV-19). NEXUM_AUTHORING comes from the preset.
target_compile_definitions(MuClient PRIVATE NEXUM_AUTHORING_ENABLED=$<BOOL:${NEXUM_AUTHORING}>)

string(REPLACE "." "\\." version_pattern "${PROJECT_VERSION}")
add_test(NAME muclient_reports_its_version COMMAND MuClient --version)
set_tests_properties(muclient_reports_its_version PROPERTIES
    PASS_REGULAR_EXPRESSION "^MuClient ${version_pattern} \\(Nexum ${version_pattern}, Nexum GridRpg ${version_pattern}\\)")

if(NEXUM_AUTHORING)
    set(expected_build_kind "development build")
else()
    set(expected_build_kind "player build")
endif()
add_test(NAME muclient_reports_the_build_kind COMMAND MuClient --version)
set_tests_properties(muclient_reports_the_build_kind PROPERTIES PASS_REGULAR_EXPRESSION "${expected_build_kind}")

add_test(NAME muclient_refuses_unknown_arguments COMMAND MuClient --unknown)
set_tests_properties(muclient_refuses_unknown_arguments PROPERTIES WILL_FAIL TRUE)
