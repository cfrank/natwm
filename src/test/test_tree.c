// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include <common/tree.h>

int test_setup(void **state)
{
        struct tree *tree = tree_create(NULL);

        if (tree == NULL) {
                return EXIT_FAILURE;
        }

        *state = tree;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        tree_destroy(*(struct tree **)state, NULL);

        return EXIT_SUCCESS;
}

static void test_tree_create(void **state)
{
        struct tree *tree = *(struct tree **)state;

        assert_int_equal(0, tree->size);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_tree_create, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
