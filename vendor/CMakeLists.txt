# All external libs

if (ENABLE_TESTING)
    # Libs required for testing
    option(WITH_EXAMPLES "Don't include cmocka examples" OFF)
    add_subdirectory(cmocka)
endif()

add_subdirectory(clog)
