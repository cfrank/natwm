// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

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

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_color_value_from_string),
                cmocka_unit_test(test_color_value_from_string_invalid),
                cmocka_unit_test(test_color_value_from_string_missing_hash),
                cmocka_unit_test(test_color_value_from_string_invalid_length),
        };

        return cmocka_run_group_tests(
                tests, global_test_setup, global_test_teardown);
}
