add_natwm_test(test_list
    SOURCES test_list.c
    LINK_LIBRARIES
        ${CMOCKA_SHARED_LIBRARY}
        core
    TEST_NAME LinkedListTest
)
