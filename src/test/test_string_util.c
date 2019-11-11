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
#include <common/string.h>
#include <common/util.h>

static void test_string_init(void **state)
{
        const char *expected_string = "Test String";
        char *result = string_init(expected_string);

        assert_string_equal(expected_string, result);
        assert_ptr_not_equal(expected_string, result);

        free(result);
}

static void test_string_append(void **state)
{
        const char *expected_string = "TestString";
        char *first_string = string_init("Test");
        char *second_string = string_init("String");

        assert_int_equal(0, string_append(&first_string, second_string));
        assert_string_equal(expected_string, first_string);

        // Make sure the null terminator is appended as well
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
        free(second_string);
}

static void test_string_append_empty_append(void **state)
{
        const char *expected_string = "Test";
        char *first_string = string_init(expected_string);

        assert_int_equal(0, string_append(&first_string, ""));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_append_empty_destination(void **state)
{
        const char *expected_string = "String";
        char *first_string = string_init("");
        char *second_string = string_init(expected_string);

        assert_int_equal(0, string_append(&first_string, second_string));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
        free(second_string);
}

static void test_string_append_char_succeeds(void **state)
{
        const char *expected_string = "Test.";
        char *first_string = string_init("Test");

        assert_int_equal(0, string_append_char(&first_string, '.'));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_append_char_empty_append(void **state)
{
        const char *expected_string = "Test";
        char *first_string = string_init(expected_string);

        assert_int_equal(0, string_append_char(&first_string, '\0'));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_append_char_empty_destination(void **state)
{
        const char *expected_string = ".";
        char *first_string = string_init("");

        assert_int_equal(0, string_append_char(&first_string, '.'));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_find_char(void **state)
{
        ssize_t expected_length = 5;
        const char *string = "Hello!";

        assert_int_equal(expected_length, string_find_char(string, '!'));
}

static void test_string_find_char_not_found(void **state)
{
        ssize_t expected_length = -1;
        const char *string = "Not Found";

        assert_int_equal(expected_length, string_find_char(string, '!'));
}

static void test_string_find_char_empty_string(void **state)
{
        ssize_t expected_length = -1;
        const char *string = "";

        assert_int_equal(expected_length, string_find_char(string, '!'));
}

static void test_string_find_char_null_string(void **state)
{
        ssize_t expected_length = -1;
        const char *string = NULL;

        assert_int_equal(expected_length, string_find_char(string, '!'));
}

static void test_string_find_first_nonspace(void **state)
{
        ssize_t expected_index = 3;
        const char *str = "   Hello world!";

        assert_int_equal(expected_index, string_find_first_nonspace(str));
}

static void test_string_find_first_nonspace_not_found(void **state)
{
        ssize_t expected_index = -1;
        const char *str = "   ";

        assert_int_equal(expected_index, string_find_first_nonspace(str));
}

static void test_string_find_first_nonspace_single_char(void **state)
{
        ssize_t expected_index = 0;
        const char *str = "H";

        assert_int_equal(expected_index, string_find_first_nonspace(str));
}

static void test_string_find_first_nonspace_null_string(void **state)
{
        ssize_t expected_index = -1;

        assert_int_equal(expected_index, string_find_first_nonspace(NULL));
}

static void test_string_find_last_nonspace(void **state)
{
        ssize_t expected_index = 11;
        const char *str = "Hello World!    ";

        assert_int_equal(expected_index, string_find_last_nonspace(str));
}

static void test_string_find_last_nonspace_not_found(void **state)
{
        ssize_t expected_index = -1;
        const char *str = "    ";

        assert_int_equal(expected_index, string_find_last_nonspace(str));
}

static void test_string_find_last_nonspace_single_char(void **state)
{
        ssize_t expected_index = 0;
        const char *str = "H";

        assert_int_equal(expected_index, string_find_last_nonspace(str));
}

static void test_string_find_last_nonspace_null_string(void **state)
{
        ssize_t expected_index = -1;

        assert_int_equal(expected_index, string_find_last_nonspace(NULL));
}

static void test_string_get_delimiter(void **state)
{
        const char *input = "Hello world! My name is computer";
        const char *expected_string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;

        ssize_t result = string_get_delimiter(input, '!', &destination, true);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_get_delimiter_not_found(void **state)
{
        const char *input = "Hello world!";
        ssize_t expected_index = -1;
        char *destination = NULL;

        ssize_t result = string_get_delimiter(input, '$', &destination, true);

        assert_int_equal(expected_index, result);
}

static void test_string_get_delimiter_first_char(void **state)
{
        const char *input = "Hello world!";
        const char *expected_string = "H";
        ssize_t expected_result = 1;
        char *destination = NULL;

        ssize_t result = string_get_delimiter(input, 'H', &destination, true);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);
        assert_int_equal('\0', destination[expected_result]);

        free(destination);
}

static void test_string_get_delimiter_empty_string(void **state)
{
        const char *input = "";
        ssize_t expected_index = -1;
        char *destination = NULL;

        ssize_t result = string_get_delimiter(input, '!', &destination, true);

        assert_int_equal(expected_index, result);
}

static void test_string_to_number(void **state)
{
        const char *input = "1520";
        intmax_t expected_number = 1520;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(0, result);
        assert_int_equal(expected_number, destination);
}

static void test_string_to_number_negative(void **state)
{
        const char *input = "-3455";
        intmax_t expected_number = -3455;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(0, result);
        assert_int_equal(expected_number, destination);
}

static void test_string_to_number_invalid_char(void **state)
{
        const char *input = "123abc";
        int expected_number = -1;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(expected_number, result);
        assert_int_equal(0, destination);
}

static void test_string_to_number_empty(void **state)
{
        const char *input = "";
        int expected_number = -1;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(expected_number, result);
        assert_int_equal(0, destination);
}

static void test_string_to_number_zero(void **state)
{
        const char *input = "0";
        int expected_result = 0;
        intmax_t expected_number = 0;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(expected_result, result);
        assert_int_equal(expected_number, destination);
}

static void test_string_to_number_single_char(void **state)
{
        const char *input = "e";
        int expected_result = -1;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(expected_result, result);
        assert_int_equal(0, destination);
}

static void test_string_to_number_double(void **state)
{
        const char *input = "55.5";
        int expected_result = -1;
        intmax_t destination = 0;

        int result = string_to_number(input, &destination);

        assert_int_equal(expected_result, result);
        assert_int_equal(0, destination);
}

static void test_string_splice(void **state)
{
        const char *input = "Hello world!";
        const char *expected_string = "world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;

        ssize_t result
                = string_splice(input, &destination, 6, (ssize_t)strlen(input));

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
        const char *input = "Test";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(input, &destination, 5, 10);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_large_end(void **state)
{
        const char *input = "Test";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(input, &destination, 0, 5);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_mismatch_start_end(void **state)
{
        const char *input = "Test";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_splice(input, &destination, 4, 1);

        assert_null(destination);
        assert_int_equal(expected_result, result);
}

static void test_string_splice_single_char(void **state)
{
        const char *input = "T";
        ssize_t expected_result = 1;
        char *destination = NULL;
        ssize_t result = string_splice(input, &destination, 0, 1);

        assert_int_equal(expected_result, result);
        assert_string_equal(input, destination);

        free(destination);
}

static void test_string_splice_zero_start_end(void **state)
{
        const char *input = "Something";
        const char *expected_string = "";
        ssize_t expected_result = 0;
        char *destination = NULL;
        ssize_t result = string_splice(input, &destination, 0, 0);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces(void **state)
{
        const char *input = "    Hello world!   ";
        const char *expected_string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(input, &destination);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_tabs(void **state)
{
        const char *input = "\tHello world!  \t";
        const char *expected_string = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(expected_string);
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(input, &destination);

        assert_int_equal(expected_result, result);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_no_spaces(void **state)
{
        const char *input = "Hello world!";
        ssize_t expected_result = (ssize_t)strlen(input);
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(input, &destination);

        assert_int_equal(expected_result, result);
        assert_string_equal(input, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_all_spaces(void **state)
{
        const char *input = " ";
        ssize_t expected_result = -1;
        char *destination = NULL;
        ssize_t result = string_strip_surrounding_spaces(input, &destination);

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
                cmocka_unit_test(test_string_init),
                cmocka_unit_test(test_string_append),
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
                cmocka_unit_test(test_string_to_number),
                cmocka_unit_test(test_string_to_number_negative),
                cmocka_unit_test(test_string_to_number_invalid_char),
                cmocka_unit_test(test_string_to_number_empty),
                cmocka_unit_test(test_string_to_number_zero),
                cmocka_unit_test(test_string_to_number_single_char),
                cmocka_unit_test(test_string_to_number_double),
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
