# AddNatwmTest
# ------------
#
# Function which adds a test
#
# Heavily based on the function ADD_CMOCKA_TEST provided
# by Cmocka, but with a few improvements
#
# Example
# -------
# add_natwm_test(example-test
#                SOURCES example-test.c
#                COMPILE_OPTIONS -g
#                LINK_LIBRARIES ${CMOCKA_SHARED_LIBRARY}
#                LINK_OPTIONS -Wl,--enable-syscall-fixup
#                TEST_NAME ExampleTest
#

function(ADD_NATWM_TEST _TARGET_NAME)
    set(one_value_arguments
        TEST_NAME
    )

    set(multi_value_arguments
        SOURCES
        COMPILE_OPTIONS
        LINK_LIBRARIES
        LINK_OPTIONS
    )

    cmake_parse_arguments(_add_natwm_test
        ""
        "${one_value_arguments}"
        "${multi_value_arguments}"
        ${ARGN}
    )

    if (NOT DEFINED _add_natwm_test_SOURCES)
        message(FATAL_ERROR "No sources provided for target ${_TARGET_NAME}")
    endif()

    add_executable(${_TARGET_NAME} ${_add_natwm_test_SOURCES})

    if (DEFINED _add_natwm_test_COMPILE_OPTIONS)
        target_compile_options(${_TARGET_NAME}
            PRIVATE ${_add_natwm_test_COMPILE_OPTIONS}
        )
    endif()

    if (DEFINED _add_natwm_test_LINK_LIBRARIES)
        target_link_libraries(${_TARGET_NAME}
            PRIVATE ${_add_natwm_test_LINK_LIBRARIES}
        )
    endif()

    if (DEFINED _add_natwm_test_LINK_OPTIONS)
        set_target_properties(${_TARGET_NAME}
            PROPERTIES LINK_FLAGS
            ${_add_natwm_test_LINK_OPTIONS}
        )
    endif()

    if (DEFINED _add_natwm_test_TEST_NAME)
        set(TEST_NAME ${_add_natwm_test_TEST_NAME})
    else()
        set(TEST_NAME ${_TARGET_NAME})
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${_TARGET_NAME})
endfunction(ADD_NATWM_TEST)
