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

#include <core/config/config.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/theme.h>

/**
 * Since theme uses logs we need to silence them
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

static void config_map_free_callback(void *data)
{
        struct config_value *value = (struct config_value *)data;

        config_value_destroy(value);
}

static void test_color_value_from_string(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        uint32_t expected_result = 16777215;
        const char *string = "#ffffff";
        struct color_value *result = NULL;

        assert_int_equal(NO_ERROR, color_value_from_string(string, &result));
        assert_string_equal(string, result->string);
        assert_int_equal(expected_result, result->color_value);

        color_value_destroy(result);
}

static void test_color_value_from_string_invalid(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *string = "abc123";
        struct color_value *result = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         color_value_from_string(string, &result));
        assert_null(result);
}

static void test_color_value_from_string_missing_hash(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        // Missing '#'
        const char *string = "ffffff";
        struct color_value *result = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         color_value_from_string(string, &result));
        assert_null(result);
}

static void test_color_value_from_string_invalid_length(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        // Invalid length
        const char *string = "fff";
        struct color_value *result = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         color_value_from_string(string, &result));
        assert_null(result);
}

static void test_border_theme_from_config(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        uint16_t expected_values[] = {
                0,
                1,
                2,
                3,
        };

        /**
         * config = [
         * 	1,
         * 	2,
         * 	3,
         * ]
         */
        const char *config_string = "config = [\n"
                                    "\t0,\n"
                                    "\t1,\n"
                                    "\t2,\n"
                                    "\t3,\n"
                                    "]\n";
        struct map *config_map
                = config_read_string(config_string, strlen(config_string));

        map_set_entry_free_function(config_map, config_map_free_callback);

        assert_non_null(config_map);

        struct border_theme *theme = NULL;

        assert_int_equal(
                NO_ERROR,
                border_theme_from_config(config_map, "config", &theme));
        assert_non_null(theme);
        assert_int_equal(expected_values[0], theme->unfocused);
        assert_int_equal(expected_values[1], theme->focused);
        assert_int_equal(expected_values[2], theme->urgent);
        assert_int_equal(expected_values[3], theme->sticky);

        map_destroy(config_map);
}

static void test_border_theme_from_config_ignore_invalid_item(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        uint16_t expected_values[] = {
                0,
                1,
                DEFAULT_BORDER_WIDTH,
                DEFAULT_BORDER_WIDTH,
        };

        // When given items of invalid types we should fallback to
        // DEFAULT_BORDER_WIDTH

        /**
         * config = [
         * 	0,
         * 	1,
         * 	"Invalid string type",
         * 	["Invalid array type"],
         * ]
         */
        const char *config_string = "config = [\n"
                                    "\t0,\n"
                                    "\t1,\n"
                                    "\t\"Invalid string type\",\n"
                                    "\t[\"Invalid string type\"],\n"
                                    "]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        map_set_entry_free_function(config_map, config_map_free_callback);

        struct border_theme *theme = NULL;

        assert_int_equal(
                NO_ERROR,
                border_theme_from_config(config_map, "config", &theme));
        assert_non_null(theme);
        assert_int_equal(expected_values[0], theme->unfocused);
        assert_int_equal(expected_values[1], theme->focused);
        assert_int_equal(expected_values[2], theme->urgent);
        assert_int_equal(expected_values[3], theme->sticky);

        border_theme_destroy(theme);
        config_destroy(config_map);
}

static void test_border_theme_from_config_invalid_length(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "config = []\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        map_set_entry_free_function(config_map, config_map_free_callback);

        struct border_theme *theme = NULL;

        assert_int_equal(
                INVALID_INPUT_ERROR,
                border_theme_from_config(config_map, "config", &theme));
        assert_null(theme);

        config_destroy(config_map);
}

static void test_border_theme_from_config_invalid_type(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "config = \"Invalid Type\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        map_set_entry_free_function(config_map, config_map_free_callback);

        struct border_theme *theme = NULL;

        assert_int_equal(
                INVALID_INPUT_ERROR,
                border_theme_from_config(config_map, "config", &theme));
        assert_null(theme);

        config_destroy(config_map);
}

static void test_border_theme_from_config_missing_config_item(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "invalid = \"Not found\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        map_set_entry_free_function(config_map, config_map_free_callback);

        struct border_theme *theme = NULL;

        assert_int_equal(
                NOT_FOUND_ERROR,
                border_theme_from_config(config_map, "config", &theme));
        assert_null(theme);

        config_destroy(config_map);
}

static void test_color_theme_from_config(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *expected_values[] = {
                "#ffffff",
                "#cc0000",
                "#000000",
                "#cccccc",
        };
        struct color_value *expected_unfocused_value = NULL;
        struct color_value *expected_focused_value = NULL;
        struct color_value *expected_urgent_value = NULL;
        struct color_value *expected_sticky_value = NULL;

        assert_int_equal(NO_ERROR,
                         color_value_from_string(expected_values[0],
                                                 &expected_unfocused_value));
        assert_int_equal(NO_ERROR,
                         color_value_from_string(expected_values[1],
                                                 &expected_focused_value));
        assert_int_equal(NO_ERROR,
                         color_value_from_string(expected_values[2],
                                                 &expected_urgent_value));
        assert_int_equal(NO_ERROR,
                         color_value_from_string(expected_values[3],
                                                 &expected_sticky_value));
        /**
         * config = [
         *     "#ffffff",
         *     "#cc0000",
         *     "#000000",
         *     "#cccccc",
         * ]
         */
        const char *config_string = "config = [\n"
                                    "\t\"#ffffff\","
                                    "\t\"#cc0000\","
                                    "\t\"#000000\","
                                    "\t\"#cccccc\","
                                    "]\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        map_set_entry_free_function(config_map, config_map_free_callback);

        assert_non_null(config_map);

        struct color_theme *result = NULL;

        assert_int_equal(
                NO_ERROR,
                color_theme_from_config(config_map, "config", &result));
        assert_non_null(result);
        assert_string_equal(expected_unfocused_value->string,
                            result->unfocused->string);
        assert_int_equal(expected_unfocused_value->color_value,
                         result->unfocused->color_value);
        assert_string_equal(expected_focused_value->string,
                            result->focused->string);
        assert_int_equal(expected_focused_value->color_value,
                         result->focused->color_value);
        assert_string_equal(expected_urgent_value->string,
                            result->urgent->string);
        assert_int_equal(expected_urgent_value->color_value,
                         result->urgent->color_value);
        assert_string_equal(expected_sticky_value->string,
                            result->sticky->string);
        assert_int_equal(expected_sticky_value->color_value,
                         result->sticky->color_value);

        color_value_destroy(expected_unfocused_value);
        color_value_destroy(expected_focused_value);
        color_value_destroy(expected_urgent_value);
        color_value_destroy(expected_sticky_value);

        color_theme_destroy(result);

        map_destroy(config_map);
}

static void test_color_theme_from_config_invalid_length(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "config = []\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct color_theme *theme = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         color_theme_from_config(config_map, "config", &theme));
        assert_null(theme);

        map_destroy(config_map);
}

static void test_color_theme_from_config_invalid_type(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "config = \"Hello world\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct color_theme *theme = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         color_theme_from_config(config_map, "config", &theme));
        assert_null(theme);

        map_destroy(config_map);
}

static void test_color_theme_from_config_missing_config_item(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        const char *config_string = "not_found = \"Invalid\"\n";
        size_t config_length = strlen(config_string);
        struct map *config_map
                = config_read_string(config_string, config_length);

        assert_non_null(config_map);

        struct color_theme *theme = NULL;

        assert_int_equal(NOT_FOUND_ERROR,
                         color_theme_from_config(config_map, "config", &theme));
        assert_null(theme);

        map_destroy(config_map);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_color_value_from_string),
                cmocka_unit_test(test_color_value_from_string_invalid),
                cmocka_unit_test(test_color_value_from_string_missing_hash),
                cmocka_unit_test(test_color_value_from_string_invalid_length),
                cmocka_unit_test(test_border_theme_from_config),
                cmocka_unit_test(
                        test_border_theme_from_config_ignore_invalid_item),
                cmocka_unit_test(test_border_theme_from_config_invalid_length),
                cmocka_unit_test(test_border_theme_from_config_invalid_type),
                cmocka_unit_test(
                        test_border_theme_from_config_missing_config_item),
                cmocka_unit_test(test_color_theme_from_config),
                cmocka_unit_test(test_color_theme_from_config_invalid_length),
                cmocka_unit_test(test_color_theme_from_config_invalid_type),
                cmocka_unit_test(
                        test_color_theme_from_config_missing_config_item),
        };

        return cmocka_run_group_tests(
                tests, global_test_setup, global_test_teardown);
}
