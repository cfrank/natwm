# Common
# Common/List
add_natwm_test(test_list
    SOURCES test_list.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME LinkedListTest
)

# Common/Map
add_natwm_test(test_map
    SOURCES test_map.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME HashMapTest
)

# Common/String
add_natwm_test(test_string_util
    SOURCES test_string_util.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME StringUtilTest
)

# Common/Stack
add_natwm_test(test_stack
    SOURCES test_stack.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME StackTest
)

# Common/Theme
add_natwm_test(test_theme
    SOURCES test_theme.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
        core
    TEST_NAME ThemeTest
)

# Common/Tree
add_natwm_test(test_tree
    SOURCES test_tree.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        common
    TEST_NAME TreeTest
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
