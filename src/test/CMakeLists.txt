add_executable(test
    test_main.c
)

set_target_properties(natwm
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    C_STANDARD 99
    C_EXTENSIONS OFF
)

target_link_libraries(test PRIVATE unity)

add_test(NAME natwm-test COMMAND test)
