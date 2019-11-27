// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include <common/stack.h>

static int test_setup(void **state)
{
        struct stack *stack = stack_create();

        if (stack == NULL) {
                return EXIT_FAILURE;
        }

        *state = stack;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        stack_destroy(*(struct stack **)state);

        return EXIT_SUCCESS;
}

static void test_stack_creation(void **state)
{
        struct stack *stack = *(struct stack **)state;

        assert_int_equal(0, stack->length);
        assert_null(stack->head);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_stack_creation, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
