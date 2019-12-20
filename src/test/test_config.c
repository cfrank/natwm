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
        assert_int_equal(value->type, STRING);
        assert_string_equal(value->data.string, expected_value);

        config_destroy(config_map);
}

static void test_config_array(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "simple.array";
        size_t expected_array_length = 3;
        intmax_t expected_values[3] = {1, 2, 3};
        const char *config_string = "simple.array = [1,2,3]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *array_item = array->values[i];

                assert_non_null(array_item);
                assert_int_equal(NUMBER, array_item->type);
                assert_int_equal(expected_values[i], array_item->data.number);
        }

        config_destroy(config_map);
}

static void test_config_nested_array(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "nested.array";
        size_t expected_array_length = 3;
        size_t expected_nested_array_length = 2;
        const char *expected_values[3][2] = {
                {"one", "two"},
                {"three", "four"},
                {"five", "six"},
        };
        /**
         * $third.value = "three"
         * $second.array = [$third.value, "four"]
         * $nested.array = [
         *     ["one", "two"],
         *     $second.array,
         *     ["five", "six"],
         * ]
         */
        const char *config_string = "$third.value = \"three\"\n"
                                    "$second.array = [$third.value, \"four\"]\n"
                                    "nested.array = [\n"
                                    "\t[\"one\", \"two\"],\n"
                                    "\t$second.array,\n"
                                    "\t[\"five\", \"six\"],\n"
                                    "]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_non_null(array);
        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *nested_value = array->values[i];

                assert_non_null(nested_value);
                assert_int_equal(ARRAY, nested_value->type);

                struct config_array *nested_array = nested_value->data.array;

                assert_non_null(nested_array);
                assert_int_equal(expected_nested_array_length,
                                 nested_array->length);
                assert_non_null(nested_array->values);

                for (size_t j = 0; j < expected_nested_array_length; ++j) {
                        struct config_value *nested_array_value
                                = nested_array->values[j];

                        assert_non_null(nested_array_value);
                        assert_int_equal(STRING, nested_array_value->type);
                        assert_string_equal(expected_values[i][j],
                                            nested_array_value->data.string);
                }
        }

        config_destroy(config_map);
}

static void test_config_nested_array_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "nested.array";
        size_t expected_array_length = 3;
        size_t expected_nested_array_length = 2;
        intmax_t expected_values[3][2] = {
                {1, 2},
                {2, 1},
                {2, 2},
        };
        /**
         * $nested.array.variable = [
         *     [1, 2],
         *     [2, 1],
         *     [2, 2],
         * ]
         *
         * nested.array = $nested.array.variable
         */
        const char *config_string = "$nested.array.variable = [\n"
                                    "\t[1, 2],\n"
                                    "\t[2, 1],\n"
                                    "\t[2, 2],\n"
                                    "]\n"
                                    "nested.array = $nested.array.variable\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_non_null(array);
        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *nested_value = array->values[i];

                assert_non_null(nested_value);
                assert_int_equal(ARRAY, nested_value->type);

                struct config_array *nested_array = nested_value->data.array;

                assert_non_null(nested_array);
                assert_int_equal(expected_nested_array_length,
                                 nested_array->length);
                assert_non_null(nested_array->values);

                for (size_t j = 0; j < expected_nested_array_length; ++j) {
                        struct config_value *nested_array_value
                                = nested_array->values[j];

                        assert_non_null(nested_array_value);
                        assert_int_equal(NUMBER, nested_array_value->type);
                        assert_int_equal(expected_values[i][j],
                                         nested_array_value->data.number);
                }
        }

        config_destroy(config_map);
}

static void test_config_array_variable_array(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "nested.array";
        size_t expected_array_length = 3;
        size_t expected_nested_array_length = 2;
        bool expected_values[3][2] = {
                {true, false},
                {false, true},
                {false, false},
        };
        const char *config_string = "$one = [true, false]\n"
                                    "$two = [false, true]\n"
                                    "$three = [false, false]\n"
                                    "nested.array = [$one, $two, $three]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_non_null(array);
        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *nested_value = array->values[i];

                assert_non_null(nested_value);
                assert_int_equal(ARRAY, nested_value->type);

                struct config_array *nested_array = nested_value->data.array;

                assert_non_null(nested_array);
                assert_int_equal(expected_nested_array_length,
                                 nested_array->length);
                assert_non_null(nested_array->values);

                for (size_t j = 0; j < expected_nested_array_length; ++j) {
                        struct config_value *nested_item
                                = nested_array->values[j];

                        assert_non_null(nested_item);
                        assert_int_equal(BOOLEAN, nested_item->type);
                        assert_int_equal(expected_values[i][j],
                                         nested_item->data.boolean);
                }
        }

        config_destroy(config_map);
}

static void test_config_array_boolean(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "bool.array";
        size_t expected_array_length = 3;
        bool expected_values[3] = {true, false, true};
        // True/False can be any case
        const char *config_string = "bool.array = [true, False, TRUE]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *array_item = array->values[i];

                assert_non_null(array_item);
                assert_int_equal(BOOLEAN, array_item->type);
                assert_int_equal(expected_values[i], array_item->data.boolean);
        }

        config_destroy(config_map);
}

static void test_config_array_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "strings";
        size_t expected_array_length = 3;
        const char *expected_values[3] = {"one", "two", "three"};
        const char *config_string
                = "$nums = [\"one\", \"two\", \"three\"]\nstrings = $nums\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *array_item = array->values[i];

                assert_non_null(array_item);
                assert_int_equal(STRING, array_item->type);
                assert_string_equal(expected_values[i],
                                    array_item->data.number);
        }

        config_destroy(config_map);
}

static void test_config_array_empty(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "empty.array";
        size_t expected_array_length = 0;
        const char *config_string = "empty.array = []\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        config_destroy(config_map);
}

static void test_config_array_invalid(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "invalid = [fail]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_null(config_map);
}

static void test_config_array_trailing_comma(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "trailing.array";
        size_t expected_array_length = 1;
        const char *expected_values[1] = {"one"};
        const char *config_string = "trailing.array = [\"one\",]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *array_item = array->values[i];

                assert_non_null(array_item);
                assert_int_equal(STRING, array_item->type);
                assert_string_equal(expected_values[i],
                                    array_item->data.number);
        }

        config_destroy(config_map);
}

static void test_config_array_trailing_comma_multiline(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "trailing.array";
        size_t expected_array_length = 3;
        const char *expected_values[3] = {"one", "two", "three"};
        /**
         * trailing.array = [
         *     "one",
         *     "two",
         *     "three",
         * ]
         */
        const char *config_string = "trailing.array = [\n"
                                    "\"one\",\n"
                                    "\"two\",\n"
                                    "\"three\",\n"
                                    "]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(ARRAY, value->type);

        struct config_array *array = value->data.array;

        assert_int_equal(expected_array_length, array->length);
        assert_non_null(array->values);

        for (size_t i = 0; i < expected_array_length; ++i) {
                struct config_value *array_item = array->values[i];

                assert_non_null(array_item);
                assert_int_equal(STRING, array_item->type);
                assert_string_equal(expected_values[i],
                                    array_item->data.number);
        }

        config_destroy(config_map);
}

static void test_config_boolean(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "works";
        const char *config_string = "works = true\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(BOOLEAN, value->type);
        assert_true(value->data.boolean);

        config_destroy(config_map);
}

static void test_config_boolean_variable(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_key = "works";
        const char *config_string = "$is_working = true\nworks = $is_working\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct config_value *value = config_find(config_map, expected_key);

        assert_non_null(value);
        assert_int_equal(BOOLEAN, value->type);
        assert_true(value->data.boolean);

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

        assert_non_null(value);
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

        assert_non_null(value);
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

        assert_non_null(value);
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
                cmocka_unit_test(test_config_array),
                cmocka_unit_test(test_config_nested_array),
                cmocka_unit_test(test_config_nested_array_variable),
                cmocka_unit_test(test_config_array_variable_array),
                cmocka_unit_test(test_config_array_boolean),
                cmocka_unit_test(test_config_array_variable),
                cmocka_unit_test(test_config_array_empty),
                cmocka_unit_test(test_config_array_invalid),
                cmocka_unit_test(test_config_array_trailing_comma),
                cmocka_unit_test(test_config_array_trailing_comma_multiline),
                cmocka_unit_test(test_config_boolean),
                cmocka_unit_test(test_config_boolean_variable),
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
