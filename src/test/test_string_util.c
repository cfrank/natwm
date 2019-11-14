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

#include <common/constants.h>
#include <common/error.h>
#include <common/string.h>

static void test_string_init(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = "Test String";
        char *result = string_init(expected_string);

        assert_string_equal(expected_string, result);
        assert_ptr_not_equal(expected_string, result);

        free(result);
}

static void test_string_append(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = "TestString";
        char *first_string = string_init("Test");
        char *second_string = string_init("String");

        assert_int_equal(NO_ERROR, string_append(&first_string, second_string));
        assert_string_equal(expected_string, first_string);

        // Make sure the null terminator is appended as well
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
        free(second_string);
}

static void test_string_append_empty_append(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = "Test";
        char *first_string = string_init(expected_string);

        assert_int_equal(NO_ERROR, string_append(&first_string, ""));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_append_empty_destination(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = "String";
        char *first_string = string_init("");
        char *second_string = string_init(expected_string);

        assert_int_equal(NO_ERROR, string_append(&first_string, second_string));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
        free(second_string);
}

static void test_string_append_char_succeeds(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = "Test.";
        char *first_string = string_init("Test");

        assert_int_equal(NO_ERROR, string_append_char(&first_string, '.'));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_append_char_empty_append(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = "Test";
        char *first_string = string_init(expected_string);

        assert_int_equal(NO_ERROR, string_append_char(&first_string, '\0'));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_append_char_empty_destination(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_string = ".";
        char *first_string = string_init("");

        assert_int_equal(NO_ERROR, string_append_char(&first_string, '.'));
        assert_string_equal(expected_string, first_string);
        assert_int_equal('\0', first_string[strlen(first_string)]);

        free(first_string);
}

static void test_string_find_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_index = 5;
        const char *string = "Hello!";
        size_t index = 0;
        enum natwm_error err = string_find_char(string, '!', &index);

        assert_int_equal(NO_ERROR, err);
        assert_int_equal(expected_index, index);
}

static void test_string_find_char_not_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *string = "Not Found";

        assert_int_equal(NOT_FOUND_ERROR, string_find_char(string, '!', NULL));
}

static void test_string_find_char_empty_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *string = "";

        assert_int_equal(NOT_FOUND_ERROR, string_find_char(string, '!', NULL));
}

static void test_string_find_char_null_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *string = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_find_char(string, '!', NULL));
}

static void test_string_find_first_nonspace(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_index = 3;
        size_t index = 0;
        const char *string = "   Hello world!";

        assert_int_equal(NO_ERROR, string_find_first_nonspace(string, &index));
        assert_int_equal(expected_index, index);
}

static void test_string_find_first_nonspace_not_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *string = "   ";
        size_t index = 0;

        assert_int_equal(NOT_FOUND_ERROR,
                         string_find_first_nonspace(string, &index));
}

static void test_string_find_first_nonspace_single_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_index = 0;
        size_t index = 0;
        const char *string = "H";

        assert_int_equal(NO_ERROR, string_find_first_nonspace(string, &index));
        assert_int_equal(expected_index, index);
}

static void test_string_find_first_nonspace_null_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t index = 0;
        assert_int_equal(INVALID_INPUT_ERROR,
                         string_find_first_nonspace(NULL, &index));
}

static void test_string_find_last_nonspace(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_index = 11;
        size_t index = 0;
        const char *string = "Hello World!    ";

        assert_int_equal(NO_ERROR, string_find_last_nonspace(string, &index));
        assert_int_equal(expected_index, index);
}

static void test_string_find_last_nonspace_not_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *string = "    ";
        size_t index = 0;

        assert_int_equal(NOT_FOUND_ERROR,
                         string_find_last_nonspace(string, &index));
}

static void test_string_find_last_nonspace_single_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_index = 0;
        size_t index = 0;
        const char *str = "H";

        assert_int_equal(NO_ERROR, string_find_last_nonspace(str, &index));
        assert_int_equal(expected_index, index);
}

static void test_string_find_last_nonspace_null_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_find_last_nonspace(NULL, NULL));
}

static void test_string_get_delimiter(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Hello world! My name is computer";
        const char *expected_string = "Hello world!";
        char *destination = NULL;
        size_t expected_length = strlen(expected_string);
        size_t length = 0;

        assert_int_equal(
                NO_ERROR,
                string_get_delimiter(input, '!', &destination, &length, true));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_get_delimiter_not_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Hello world!";
        char *destination = NULL;

        assert_int_equal(
                NOT_FOUND_ERROR,
                string_get_delimiter(input, '$', &destination, NULL, true));
}

static void test_string_get_delimiter_first_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Hello world!";
        const char *expected_string = "H";
        char *destination = NULL;
        size_t expected_length = 1;
        size_t length = 0;

        assert_int_equal(
                NO_ERROR,
                string_get_delimiter(input, 'H', &destination, &length, true));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);
        assert_int_equal('\0', destination[length]);

        free(destination);
}

static void test_string_get_delimiter_empty_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "";
        char *destination = NULL;

        assert_int_equal(
                NOT_FOUND_ERROR,
                string_get_delimiter(input, '!', &destination, NULL, true));
}

static void test_string_to_number(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "1520";
        intmax_t expected_number = 1520;
        intmax_t destination = 0;

        assert_int_equal(NO_ERROR, string_to_number(input, &destination));
        assert_int_equal(expected_number, destination);
}

static void test_string_to_number_negative(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "-3455";
        intmax_t expected_number = -3455;
        intmax_t destination = 0;

        assert_int_equal(NO_ERROR, string_to_number(input, &destination));
        assert_int_equal(expected_number, destination);
}

static void test_string_to_number_invalid_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "123abc";
        intmax_t destination = 0;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_to_number(input, &destination));
        assert_int_equal(0, destination);
}

static void test_string_to_number_empty(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "";
        intmax_t destination = 0;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_to_number(input, &destination));
        assert_int_equal(0, destination);
}

static void test_string_to_number_zero(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "0";
        intmax_t destination = 0;
        intmax_t expected_number = 0;

        assert_int_equal(NO_ERROR, string_to_number(input, &destination));

        assert_int_equal(expected_number, destination);
}

static void test_string_to_number_single_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "e";
        intmax_t destination = 0;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_to_number(input, &destination));
}

static void test_string_to_number_double(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "55.5";
        intmax_t destination = 0;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_to_number(input, &destination));
}

static void test_string_splice(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Hello world!";
        const char *expected_string = "world!";
        char *destination = NULL;
        size_t expected_length = strlen(expected_string);
        size_t length = 0;

        assert_int_equal(
                NO_ERROR,
                string_splice(input, 6, strlen(input), &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);
        assert_int_equal('\0', destination[length]);

        free(destination);
}

static void test_string_splice_null_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        char *destination = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_splice(NULL, 0, 0, &destination, NULL));
}

static void test_string_splice_large_start(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Test";
        char *destination = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_splice(input, 5, 10, &destination, NULL));
}

static void test_string_splice_large_end(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Test";
        char *destination = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_splice(input, 0, 5, &destination, NULL));
}

static void test_string_splice_mismatch_start_end(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Test";
        char *destination = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         string_splice(input, 4, 1, &destination, NULL));
}

static void test_string_splice_single_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "T";
        char *destination = NULL;
        size_t expected_length = 1;
        size_t length = 0;

        assert_int_equal(NO_ERROR,
                         string_splice(input, 0, 1, &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(input, destination);

        free(destination);
}

static void test_string_splice_zero_start_end(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Something";
        const char *expected_string = "";
        char *destination = NULL;
        size_t expected_length = 0;
        size_t length = 0;

        assert_int_equal(NO_ERROR,
                         string_splice(input, 0, 0, &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = " Hello world! ";
        const char *expected_string = "Hello world!";
        char *destination = NULL;
        size_t expected_length = strlen(expected_string);
        size_t length = 0;

        assert_int_equal(
                NO_ERROR,
                string_strip_surrounding_spaces(input, &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_tabs(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "\tHello world!  \t";
        const char *expected_string = "Hello world!";
        char *destination = NULL;
        size_t expected_length = strlen(expected_string);
        size_t length = 0;

        assert_int_equal(
                NO_ERROR,
                string_strip_surrounding_spaces(input, &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_no_spaces(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = "Hello world!";
        char *destination = NULL;
        size_t expected_length = strlen(input);
        size_t length = 0;

        assert_int_equal(
                NO_ERROR,
                string_strip_surrounding_spaces(input, &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(input, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_single_char(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = " A ";
        const char *expected_string = "A";
        char *destination = NULL;
        size_t length = 0;
        size_t expected_length = 1;

        assert_int_equal(
                NO_ERROR,
                string_strip_surrounding_spaces(input, &destination, &length));

        assert_int_equal(expected_length, length);
        assert_string_equal(expected_string, destination);

        free(destination);
}

static void test_string_strip_surrounding_spaces_all_spaces(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = " ";
        char *destination = NULL;

        assert_int_equal(
                NOT_FOUND_ERROR,
                string_strip_surrounding_spaces(input, &destination, NULL));
}

static void test_string_strip_surrounding_spaces_null_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *input = NULL;
        char *destination = NULL;

        assert_int_equal(
                INVALID_INPUT_ERROR,
                string_strip_surrounding_spaces(input, &destination, NULL));
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
                        test_string_strip_surrounding_spaces_single_char),
                cmocka_unit_test(
                        test_string_strip_surrounding_spaces_all_spaces),
                cmocka_unit_test(
                        test_string_strip_surrounding_spaces_null_string),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
