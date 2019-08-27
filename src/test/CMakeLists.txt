# Common
add_natwm_test(test_string_util
    SOURCES test_string_util.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME StringUtilTest
)

# Core
# Core/Config
add_natwm_test(test_config
    SOURCES test_config.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
        core
    TEST_NAME ConfigTest
)

# Core/List
add_natwm_test(test_list
    SOURCES test_list.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        core
    TEST_NAME LinkedListTest
)

add_natwm_test(test_map
    SOURCES test_map.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        core
    TEST_NAME HashMapTest
)
