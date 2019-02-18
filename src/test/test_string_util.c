// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>
#include <common/util.h>

static void test_alloc_string_succeeds(void **state)
{
        const char *expected_result = "Test String";
        char *result = alloc_string(expected_result);

        assert_string_equal(expected_result, result);
        assert_ptr_not_equal(expected_result, result);

        free(result);
}

static void test_string_append_succeeds(void **state)
{
        const char *expected_result = "TestString";
        char *first = alloc_string("Test");
        char *second = alloc_string("String");

        assert_int_equal(0, string_append(&first, second));
        assert_string_equal(expected_result, first);
        assert_ptr_not_equal(expected_result, first);

        // Make sure the null terminator is appended as well
        assert_int_equal('\0', first[strlen(first)]);

        free(first);
        free(second);
}

static void test_string_append_empty_append(void **state)
{
        const char *expected_result = "Test";
        char *first = alloc_string(expected_result);

        assert_int_equal(0, string_append(&first, ""));
        assert_string_equal(expected_result, first);
        assert_int_equal('\0', first[strlen(first)]);

        free(first);
}

static void test_string_append_empty_destination(void **state)
{
        const char *expected_result = "String";
        char *first = alloc_string("");
        char *second = alloc_string(expected_result);

        assert_int_equal(0, string_append(&first, second));
        assert_string_equal(expected_result, first);
        assert_int_equal('\0', first[strlen(first)]);

        free(first);
        free(second);
}

static void test_string_append_char_succeeds(void **state)
{
        const char *expected_result = "Test.";
        char *first = alloc_string("Test");

        assert_int_equal(0, string_append_char(&first, '.'));
        assert_string_equal(expected_result, first);
        assert_int_equal('\0', first[strlen(first)]);

        free(first);
}

static void test_string_append_char_empty_append(void **state)
{
        const char *expected_result = "Test";
        char *first = alloc_string(expected_result);

        assert_int_equal(0, string_append_char(&first, '\0'));
        assert_string_equal(expected_result, first);
        assert_int_equal('\0', first[strlen(first)]);

        free(first);
}

static void test_string_append_char_empty_destination(void **state)
{
        const char *expected_result = ".";
        char *first = alloc_string("");

        assert_int_equal(0, string_append_char(&first, '.'));
        assert_string_equal(expected_result, first);
        assert_int_equal('\0', first[strlen(first)]);

        free(first);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_alloc_string_succeeds),
                cmocka_unit_test(test_string_append_succeeds),
                cmocka_unit_test(test_string_append_empty_append),
                cmocka_unit_test(test_string_append_empty_destination),
                cmocka_unit_test(test_string_append_char_succeeds),
                cmocka_unit_test(test_string_append_char_empty_append),
                cmocka_unit_test(test_string_append_char_empty_destination),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
