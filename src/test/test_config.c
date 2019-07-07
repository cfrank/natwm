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
        const char *test_key = "name";
        const char *expected_value = "testing";
        const char *config = "$test = \"testing\"\nname = $test\n";
        size_t config_length = strlen(config);
        struct config_list *result
                = initialize_config_string(config, config_length);
        struct config_value *result_value = config_list_find(result, test_key);

        assert_non_null(result);
        assert_non_null(result_value);
        assert_int_equal(result_value->type, STRING);
        assert_string_equal(result_value->data.string, expected_value);

        destroy_config_list(result);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_config_simple_config),
        };

        return cmocka_run_group_tests(
                tests, global_test_setup, global_test_teardown);
}
