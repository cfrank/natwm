// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <common/constants.h>
#include <common/logger.h>
#include <core/config/config.h>

/**
 * Since config uses logs we need to silence them
 */
static int global_test_setup(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        initialize_logger(false);

        // Logs will now be noops
        set_logging_quiet(natwm_logger, true);

        return EXIT_SUCCESS;
}

static int global_test_teardown(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        destroy_logger(natwm_logger);

        return EXIT_SUCCESS;
}

static void test_config_simple_config(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "name";
        const char *expected_value = "John";
        const char *config_string = "name = \"John\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);
        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(value->key, expected_key);
        assert_int_equal(value->type, STRING);
        assert_string_equal(value->data.string, expected_value);

        config_destroy(config_map);
}

static void test_config_number_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "age";
        intmax_t expected_value = 100;
        const char *config_string = "$test = 100\nage = $test\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(value->key, expected_key);
        assert_int_equal(value->type, NUMBER);
        assert_int_equal(value->data.number, expected_value);

        config_destroy(config_map);
}

static void test_config_string_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "name";
        const char *expected_value = "testing";
        const char *config_string = "$test = \"testing\"\nname = $test\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(value->key, expected_key);
        assert_int_equal(value->type, STRING);
        assert_string_equal(value->data.string, expected_value);

        config_destroy(config_map);
}

static void test_config_comment(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_result = 0;
        const char *config_string = "# An example comment\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(expected_result, config_map->bucket_count);

        config_destroy(config_map);
}

static void test_config_double_definition(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "test";
        const char *expected_value = "first";
        const char *config_string
                = "$first = \"first\"\n$second = $first\ntest = $second\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(expected_key, value->key);
        assert_int_equal(STRING, value->type);
        assert_string_equal(expected_value, value->data.string);

        config_destroy(config_map);
}

static void test_config_unset_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "test = $undefined\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_number(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "$number = not a number\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_single_quotes(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "$string = 'invalid'\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "$invalid =\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_item(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "invalid = \n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_double(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "$number = 2.3\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_find_number(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        intmax_t expected_result = 14;
        intmax_t result = 0;
        const char *config_string = "test_number = 14\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(
                NO_ERROR,
                config_find_number(config_map, "test_number", &result));
        assert_int_equal(expected_result, result);

        config_destroy(config_map);
}

static void test_config_find_number_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        intmax_t result = 0;
        const char *config_string = "test_number = \"Not a number\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(
                INVALID_INPUT_ERROR,
                config_find_number(config_map, "test_number", &result));
        assert_int_equal(0, result);

        config_destroy(config_map);
}

static void test_config_find_number_null(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        intmax_t result = 0;
        const char *config_string = "test_number = 14\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(NOT_FOUND_ERROR,
                         config_find_number(config_map, NULL, &result));
        assert_int_equal(0, result);

        config_destroy(config_map);
}

static void test_config_find_number_not_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        intmax_t result = 0;
        const char *config_string = "test_number = 14\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(NOT_FOUND_ERROR,
                         config_find_number(config_map, "not_found", &result));
        assert_int_equal(0, result);

        config_destroy(config_map);
}

static void test_config_find_number_fallback(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        intmax_t expected_result = 14;
        const char *config_string = "test_number = 14\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(expected_result,
                         config_find_number_fallback(
                                 config_map, "not_found", expected_result));

        config_destroy(config_map);
}

static void test_config_find_number_fallback_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        intmax_t expected_result = 14;
        const char *config_string = "test_number = 14\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(
                expected_result,
                config_find_number_fallback(config_map, "test_number", 1000));

        config_destroy(config_map);
}

static void test_config_find_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_result = "Hello Test!";
        const char *config_string = "test_string = \"Hello Test!\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        const char *result = config_find_string(config_map, "test_string");

        assert_non_null(result);
        assert_string_equal(expected_result, result);

        config_destroy(config_map);
}

static void test_config_find_string_number(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "test_string = 14\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        const char *result = config_find_string(config_map, "test_string");

        assert_null(result);

        config_destroy(config_map);
}

static void test_config_find_string_null(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "test_string = \"Hello\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        const char *result = config_find_string(config_map, NULL);

        assert_null(result);

        config_destroy(config_map);
}

static void test_config_find_string_not_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "not_found = \"Hello\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        const char *result = config_find_string(config_map, "found");

        assert_null(result);

        config_destroy(config_map);
}

static void test_config_find_string_fallback(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_result = "Fallback Value";
        const char *config_string = "not_found = \"Hello\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        const char *result = config_find_string_fallback(
                config_map, "found", expected_result);

        assert_non_null(result);
        assert_string_equal(expected_result, result);

        config_destroy(config_map);
}

static void test_config_find_string_fallback_found(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_result = "Hello String!";
        const char *config_string = "found = \"Hello String!\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        const char *result = config_find_string_fallback(
                config_map, "found", "Fallback Value");

        assert_non_null(result);
        assert_string_equal(expected_result, result);

        config_destroy(config_map);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_config_simple_config),
                cmocka_unit_test(test_config_number_variable),
                cmocka_unit_test(test_config_string_variable),
                cmocka_unit_test(test_config_comment),
                cmocka_unit_test(test_config_double_definition),
                cmocka_unit_test(test_config_unset_variable),
                cmocka_unit_test(test_config_invalid_number),
                cmocka_unit_test(test_config_invalid_single_quotes),
                cmocka_unit_test(test_config_invalid_variable),
                cmocka_unit_test(test_config_invalid_item),
                cmocka_unit_test(test_config_invalid_double),
                cmocka_unit_test(test_config_find_number),
                cmocka_unit_test(test_config_find_number_string),
                cmocka_unit_test(test_config_find_number_null),
                cmocka_unit_test(test_config_find_number_not_found),
                cmocka_unit_test(test_config_find_number_fallback),
                cmocka_unit_test(test_config_find_number_fallback_found),
                cmocka_unit_test(test_config_find_string),
                cmocka_unit_test(test_config_find_string_number),
                cmocka_unit_test(test_config_find_string_null),
                cmocka_unit_test(test_config_find_string_not_found),
                cmocka_unit_test(test_config_find_string_fallback),
                cmocka_unit_test(test_config_find_string_fallback_found),
        };

        return cmocka_run_group_tests(
                tests, global_test_setup, global_test_teardown);
}
