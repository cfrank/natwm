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

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &expected_data));
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
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));

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
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(CAPACITY_ERROR, tree_insert(tree, NULL, NULL));
}

static void test_tree_insert_under_leaf(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        size_t expected_data_third = 3;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(
                NO_ERROR,
                tree_insert(tree, tree->root->left, &expected_data_third));

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

static bool leaf_compare(struct leaf *one, struct leaf *two)
{
        function_called();

        return one->data == two->data;
}

static void test_tree_find_parent(void **state)
{
        expect_function_call(leaf_compare);

        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        const struct leaf *parent = NULL;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(2, tree->size);
        assert_non_null(tree->root); // The parent we want
        assert_non_null(tree->root->left); // Going to use this to find it
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->data);
        assert_int_equal(
                NO_ERROR,
                tree_find_parent(
                        tree, tree->root->left, leaf_compare, &parent));
        assert_non_null(parent);
        assert_ptr_equal(tree->root, parent);
        assert_int_equal(expected_data_first, *(size_t *)parent->left->data);
}

static void test_tree_find_parent_root_single(void **state)
{
        struct tree *tree = *(struct tree **)state;
        const struct leaf *parent = NULL;
        size_t data = 14;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));

        expect_function_calls(leaf_compare, (int)tree->size);

        assert_int_equal(
                NOT_FOUND_ERROR,
                tree_find_parent(tree, tree->root, leaf_compare, &parent));
        assert_null(parent);
}

// When searching for a leaf that has no data we should return an error since
// there is no data to compare.
//
// This is an assumption - but for the use cases I can think of right now, it
// should be fine
static void test_tree_find_parent_empty_data(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data = 14;
        const struct leaf *parent = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(
                INVALID_INPUT_ERROR,
                tree_find_parent(tree, tree->root, leaf_compare, &parent));
        assert_null(parent);
}

static void test_tree_find_parent_null(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data = 14;
        const struct leaf *parent = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(INVALID_INPUT_ERROR,
                         tree_find_parent(tree, NULL, leaf_compare, &parent));
        assert_null(parent);
}

static void test_tree_find_parent_null_compare(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data = 14;
        const struct leaf *parent = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(NO_ERROR, tree_insert(tree, tree->root->left, &data));
        assert_non_null(tree->root->left->left);
        assert_non_null(tree->root->left->left->data);
        assert_int_equal(
                INVALID_INPUT_ERROR,
                tree_find_parent(tree, tree->root->left->left, NULL, &parent));
        assert_null(parent);
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
                cmocka_unit_test_setup_teardown(
                        test_tree_find_parent, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_find_parent_root_single,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_find_parent_empty_data,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_find_parent_null, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_find_parent_null_compare,
                        test_setup,
                        test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
