// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>
#include <common/logger.h>
#include <core/config.h>

/**
 * Since config uses logs we need to silence them
 */
static int global_test_setup(void **state)
{
        initialize_logger(false);

        // Logs will now be noops
        set_logging_quiet(natwm_logger, true);

        return EXIT_SUCCESS;
}

static int global_test_teardown(void **state)
{
        destroy_logger(natwm_logger);

        return EXIT_SUCCESS;
}

static void test_config_simple_config(void **state)
{
        const char *expected_key = "name";
        const char *expected_value = "John";
        const char *config_string = "name = \"John\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);
        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(value->key, expected_key);
        assert_int_equal(value->type, STRING);
        assert_string_equal(value->data.string, expected_value);

        destroy_config(config_map);
}

static void test_config_number_variable(void **state)
{
        const char *expected_key = "age";
        intmax_t expected_value = 100;
        const char *config_string = "$test = 100\nage = $test\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);
        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(value->key, expected_key);
        assert_int_equal(value->type, NUMBER);
        assert_int_equal(value->data.number, expected_value);

        destroy_config(config_map);
}

static void test_config_string_variable(void **state)
{
        const char *expected_key = "name";
        const char *expected_value = "testing";
        const char *config_string = "$test = \"testing\"\nname = $test\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);
        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(value->key, expected_key);
        assert_int_equal(value->type, STRING);
        assert_string_equal(value->data.string, expected_value);

        destroy_config(config_map);
}

static void test_config_comment(void **state)
{
        size_t expected_result = 0;
        const char *config_string = "// An example comment\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_non_null(config_map);
        assert_int_equal(expected_result, config_map->bucket_count);

        destroy_config(config_map);
}

static void test_config_double_definition(void **state)
{
        const char *expected_key = "test";
        const char *expected_value = "first";
        const char *config_string
                = "$first = \"first\"\n$second = $first\ntest = $second\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);
        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(config_map);
        assert_non_null(value);
        assert_string_equal(expected_key, value->key);
        assert_int_equal(STRING, value->type);
        assert_string_equal(expected_value, value->data.string);

        destroy_config(config_map);
}

static void test_config_unset_variable(void **state)
{
        const char *config_string = "test = $undefined\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_number(void **state)
{
        const char *config_string = "$number = not a number\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_single_quotes(void **state)
{
        const char *config_string = "$string = 'invalid'\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_variable(void **state)
{
        const char *config_string = "$invalid =\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_item(void **state)
{
        const char *config_string = "invalid = \n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_invalid_double(void **state)
{
        const char *config_string = "$number = 2.3\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = read_config_string(config_string, config_length);

        assert_null(config_map);
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
        };

        return cmocka_run_group_tests(
                tests, global_test_setup, global_test_teardown);
}
