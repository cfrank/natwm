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

        assert_non_null(tree);
        assert_non_null(tree->root);
        assert_int_equal(0, tree->size);
        assert_null(tree->root->data);
        assert_null(tree->root->left);
        assert_null(tree->root->right);
}

static void test_tree_create_initial_data(void **state)
{
        // We will not initialize this test case with a tree
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_data = 14;
        struct tree *tree = tree_create(&expected_data);

        assert_int_equal(1, tree->size);
        assert_non_null(tree->root);
        assert_non_null(tree->root->data);
        assert_ptr_equal(&expected_data, tree->root->data);
        assert_int_equal(expected_data, *(size_t *)tree->root->data);

        tree_destroy(tree, NULL);
}

static void test_leaf_create(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_data = 14;
        struct leaf *leaf = leaf_create(&expected_data);

        assert_non_null(leaf);
        assert_non_null(leaf->data);
        assert_null(leaf->left);
        assert_null(leaf->right);
        assert_ptr_equal(&expected_data, leaf->data);
        assert_int_equal(expected_data, *(size_t *)leaf->data);

        leaf_destroy(leaf);
}

static void test_tree_insert(void **state)
{
        size_t expected_data = 14;
        struct tree *tree = *(struct tree **)state;

        assert_int_equal(NO_ERROR, tree_insert(tree, &expected_data, NULL));
        assert_int_equal(1, tree->size);
        assert_non_null(tree->root->data);
        assert_null(tree->root->left);
        assert_null(tree->root->right);
        assert_ptr_equal(&expected_data, tree->root->data);
        assert_int_equal(expected_data, *(size_t *)tree->root->data);
}

static void test_tree_insert_non_empty(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, &expected_data_first, NULL));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, &expected_data_second, NULL));

        assert_int_equal(2, tree->size);
        assert_null(tree->root->data);
        assert_non_null(tree->root->left);
        assert_non_null(tree->root->right);
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->data);
        assert_int_equal(expected_data_second,
                         *(size_t *)tree->root->right->data);
}

static void test_tree_insert_full_error(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, &expected_data_first, NULL));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, &expected_data_second, NULL));
        assert_int_equal(CAPACITY_ERROR, tree_insert(tree, NULL, NULL));
}

static void test_tree_insert_under_leaf(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        size_t expected_data_third = 3;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, &expected_data_first, NULL));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, &expected_data_second, NULL));
        assert_int_equal(
                NO_ERROR,
                tree_insert(tree, &expected_data_third, tree->root->left));

        assert_null(tree->root->data);
        assert_non_null(tree->root->left);
        assert_non_null(tree->root->right);
        assert_null(tree->root->left->data);
        assert_non_null(tree->root->left->left);
        assert_non_null(tree->root->left->right);
        assert_int_equal(expected_data_second,
                         *(size_t *)tree->root->right->data);
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->left->data);
        assert_int_equal(expected_data_third,
                         *(size_t *)tree->root->left->right->data);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_tree_create, test_setup, test_teardown),
                cmocka_unit_test(test_tree_create_initial_data),
                cmocka_unit_test(test_leaf_create),
                cmocka_unit_test_setup_teardown(
                        test_tree_insert, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_insert_non_empty, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_insert_full_error, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_insert_under_leaf, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
