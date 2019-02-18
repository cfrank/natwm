# Common
add_natwm_test(test_string_util
    SOURCES test_string_util.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME StringUtilTest
)

# Core
add_natwm_test(test_list
    SOURCES test_list.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        core
    TEST_NAME LinkedListTest
)
