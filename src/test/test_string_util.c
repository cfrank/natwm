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

static void test_string_find_char(void **state)
{
        ssize_t expected_result = 5;
        const char *str = "Hello!";

        assert_int_equal(expected_result, string_find_char(str, '!'));
}

static void test_string_find_char_not_found(void **state)
{
        ssize_t expected_result = -1;
        const char *str = "Not Found";

        assert_int_equal(expected_result, string_find_char(str, '!'));
}

static void test_string_find_char_empty_string(void **state)
{
        ssize_t expected_result = -1;
        const char *str = "";

        assert_int_equal(expected_result, string_find_char(str, '!'));
}

static void test_string_find_char_null_string(void **state)
{
        ssize_t expected_result = -1;
        const char *str = NULL;

        assert_int_equal(expected_result, string_find_char(str, '!'));
}

static void test_string_find_first_nonspace(void **state)
{
        ssize_t expected_result = 3;
        const char *str = "   Hello world!";

        assert_int_equal(expected_result, string_find_first_nonspace(str));
}

static void test_string_find_first_nonspace_not_found(void **state)
{
        ssize_t expected_result = -1;
        const char *str = "   ";

        assert_int_equal(expected_result, string_find_first_nonspace(str));
}

static void test_string_find_first_nonspace_single_char(void **state)
{
        ssize_t expected_result = 0;
        const char *str = "H";

        assert_int_equal(expected_result, string_find_first_nonspace(str));
}

static void test_string_find_first_nonspace_null_string(void **state)
{
        ssize_t expected_result = -1;

        assert_int_equal(expected_result, string_find_first_nonspace(NULL));
}

static void test_string_find_last_nonspace(void **state)
{
        ssize_t expected_result = 11;
        const char *str = "Hello World!    ";

        assert_int_equal(expected_result, string_find_last_nonspace(str));
}

static void test_string_find_last_nonspace_not_found(void **state)
{
        ssize_t expected_result = -1;
        const char *str = "    ";

        assert_int_equal(expected_result, string_find_last_nonspace(str));
}

static void test_string_find_last_nonspace_single_char(void **state)
{
        ssize_t expected_result = 0;
        const char *str = "H";

        assert_int_equal(expected_result, string_find_last_nonspace(str));
}

static void test_string_find_last_nonspace_null_string(void **state)
{
        ssize_t expected_result = -1;

        assert_int_equal(expected_result, string_find_last_nonspace(NULL));
}

static void test_string_get_delimiter(void **state)
{
        const char *string = "Hello world! My name is computer";
        const char *expected_string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;

        ssize_t result = string_get_delimiter(string, '!', &destination, true);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_get_delimiter_not_found(void **state)
{
        const char *string = "Hello world!";
        ssize_t expected_result = -1;
        char *destination = NULL;

        ssize_t result = string_get_delimiter(string, '$', &destination, true);

        assert_int_equal(expected_result, result);
}

static void test_string_get_delimiter_first_char(void **state)
{
        const char *string = "Hello world!";
        const char *expected_string = "H";
        ssize_t expected_result = 1;
        char *destination = NULL;

        ssize_t result = string_get_delimiter(string, 'H', &destination, true);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);
        assert_int_equal('\0', destination[expected_result]);

        free(destination);
}

static void test_string_get_delimiter_empty_string(void **state)
{
        const char *string = "";
        ssize_t expected_result = -1;
        char *destination = NULL;

        ssize_t result = string_get_delimiter(string, '!', &destination, true);

        assert_int_equal(expected_result, result);
}

static void test_string_splice(void **state)
{
        const char *string = "Hello world!";
        const char *expected_string = "world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;

        ssize_t result = string_splice(
                string, &destination, 6, (ssize_t)strlen(string));

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);
        assert_int_equal('\0', destination[(size_t)expected_result]);

        free(destination);
}

static void test_string_splice_null_string(void **state)
{
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(NULL, &destination, 0, 0);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_large_start(void **state)
{
        const char *string = "Test";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(string, &destination, 5, 10);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_large_end(void **state)
{
        const char *string = "Test";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(string, &destination, 0, 5);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_mismatch_start_end(void **state)
{
        const char *string = "Test";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(string, &destination, 4, 1);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_single_char(void **state)
{
        const char *string = "T";
        ssize_t expected_result = 1;
        char *destination = NULL;
        ssize_t result = string_splice(string, &destination, 0, 1);

        assert_int_equal(expected_result, result);
        assert_string_equal(string, destination);

        free(destination);
}

static void test_string_splice_zero_start_end(void **state)
{
        const char *string = "Something";
        const char *expected_string = "";
        ssize_t expected_result = 0;
        char *destination = NULL;
        ssize_t result = string_splice(string, &destination, 0, 0);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces(void **state)
{
        const char *string = "    Hello world!   ";
        const char *expected_string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(string, &destination);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_tabs(void **state)
{
        const char *string = "\tHello world!  \t";
        const char *expected_string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(string, &destination);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_no_spaces(void **state)
{
        const char *string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(string);
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(string, &destination);

        assert_int_equal(expected_result, result);
        assert_string_equal(string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_all_spaces(void **state)
{
        const char *string = " ";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(string, &destination);

        assert_int_equal(expected_result, result);
}

static void test_string_strip_surrounding_spaces_null_string(void **state)
{
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(NULL, &destination);

        assert_int_equal(expected_result, result);
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
                cmocka_unit_test(test_string_find_char),
                cmocka_unit_test(test_string_find_char_not_found),
                cmocka_unit_test(test_string_find_char_empty_string),
                cmocka_unit_test(test_string_find_char_null_string),
                cmocka_unit_test(test_string_find_first_nonspace),
                cmocka_unit_test(test_string_find_first_nonspace_not_found),
                cmocka_unit_test(test_string_find_first_nonspace_single_char),
                cmocka_unit_test(test_string_find_first_nonspace_null_string),
                cmocka_unit_test(test_string_find_last_nonspace),
                cmocka_unit_test(test_string_find_last_nonspace_not_found),
                cmocka_unit_test(test_string_find_last_nonspace_single_char),
                cmocka_unit_test(test_string_find_last_nonspace_null_string),
                cmocka_unit_test(test_string_get_delimiter),
                cmocka_unit_test(test_string_get_delimiter_not_found),
                cmocka_unit_test(test_string_get_delimiter_first_char),
                cmocka_unit_test(test_string_get_delimiter_empty_string),
                cmocka_unit_test(test_string_splice),
                cmocka_unit_test(test_string_splice_null_string),
                cmocka_unit_test(test_string_splice_large_start),
                cmocka_unit_test(test_string_splice_large_end),
                cmocka_unit_test(test_string_splice_mismatch_start_end),
                cmocka_unit_test(test_string_splice_single_char),
                cmocka_unit_test(test_string_splice_zero_start_end),
                cmocka_unit_test(test_string_strip_surrounding_spaces),
                cmocka_unit_test(test_string_strip_surrounding_spaces_tabs),
                cmocka_unit_test(
                        test_string_strip_surrounding_spaces_no_spaces),
                cmocka_unit_test(
                        test_string_strip_surrounding_spaces_all_spaces),
                cmocka_unit_test(
                        test_string_strip_surrounding_spaces_null_string),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
