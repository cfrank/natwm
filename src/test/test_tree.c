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

#include <common/constants.h>
#include <common/tree.h>

struct non_primitive_type {
        const char *string;
        size_t number;
};

static struct non_primitive_type *create_npt(const char *string, size_t number)
{
        struct non_primitive_type *npt
                = malloc(sizeof(struct non_primitive_type));

        if (npt == NULL) {
                return NULL;
        }

        npt->string = string;
        npt->number = number;

        return npt;
}

static void non_primitive_type_free_data(const void *data)
{
        function_called();

        if (!data) {
                return;
        }

        struct non_primitive_type *type = (struct non_primitive_type *)data;

        free(type);
}

static void non_primitive_leaf_free(struct leaf *leaf)
{
        function_called();

        if (!leaf) {
                return;
        }

        if (leaf->data) {
                struct non_primitive_type *data
                        = (struct non_primitive_type *)leaf->data;

                free(data);
        }

        leaf_destroy(leaf);
}

static bool non_primitive_type_compare(const void *one, const void *two)
{
        function_called();

        const struct non_primitive_type *type_one
                = (const struct non_primitive_type *)one;
        const struct non_primitive_type *type_two
                = (const struct non_primitive_type *)two;

        if (one == NULL || two == NULL) {
                return false;
        }

        if (type_one->number != type_two->number) {
                return false;
        }

        if (strcmp(type_one->string, type_two->string) != 0) {
                return false;
        }

        return true;
}

static bool primitive_leaf_compare(const void *one, const void *two)
{
        function_called();

        if (one == NULL || two == NULL) {
                return false;
        }

        if (*(size_t *)one == *(size_t *)two) {
                return true;
        }

        return false;
}

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
        assert_null(tree->root->parent);
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
        assert_null(leaf->parent);
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
        assert_null(tree->root->parent);
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
        assert_null(tree->root->parent);
        assert_non_null(tree->root->left);
        assert_non_null(tree->root->left->parent);
        assert_ptr_equal(tree->root, tree->root->left->parent);
        assert_non_null(tree->root->right);
        assert_non_null(tree->root->right->parent);
        assert_ptr_equal(tree->root, tree->root->right->parent);
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

static void test_tree_remove(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        size_t expected_data_third = 3;
        struct leaf *affected_leaf = NULL;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(
                NO_ERROR,
                tree_insert(tree, tree->root->left, &expected_data_third));
        assert_int_equal(3, tree->size);
        assert_int_equal(
                NO_ERROR,
                tree_remove(
                        tree, tree->root->left->right, NULL, &affected_leaf));
        assert_int_equal(2, tree->size);
        assert_non_null(tree->root->left->data);
        assert_null(tree->root->left->left);
        assert_null(tree->root->left->right);
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->data);
        assert_int_equal(expected_data_second,
                         *(size_t *)tree->root->right->data);
        assert_non_null(affected_leaf);
        assert_ptr_equal(affected_leaf, tree->root->left);
}

static void test_tree_remove_siblingless(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        size_t expected_data_third = 3;
        struct leaf *affected_leaf = NULL;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(
                NO_ERROR,
                tree_insert(tree, tree->root->left, &expected_data_third));
        assert_int_equal(3, tree->size);
        assert_int_equal(expected_data_second,
                         *(size_t *)tree->root->right->data);
        assert_int_equal(
                NO_ERROR,
                tree_remove(tree, tree->root->right, NULL, &affected_leaf));
        assert_int_equal(2, tree->size);
        assert_non_null(tree->root->right);
        assert_non_null(tree->root->right->data);
        assert_int_equal(expected_data_third,
                         *(size_t *)tree->root->right->data);
        assert_ptr_equal(affected_leaf, tree->root);
}

static void test_tree_remove_childless_root(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data = 14;
        struct leaf *affected_leaf = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &expected_data));
        assert_int_equal(NO_ERROR,
                         tree_remove(tree, tree->root, NULL, &affected_leaf));
        assert_int_equal(0, tree->size);
        assert_null(tree->root->data);
        assert_null(tree->root->left);
        assert_null(tree->root->right);
        assert_non_null(affected_leaf);
        assert_ptr_equal(affected_leaf, tree->root);
}

static void test_tree_remove_parent_root(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        struct leaf *affected_leaf = NULL;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(INVALID_INPUT_ERROR,
                         tree_remove(tree, tree->root, NULL, &affected_leaf));
        assert_int_equal(2, tree->size);
        assert_null(tree->root->data);
        assert_non_null(tree->root->left);
        assert_non_null(tree->root->right);
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->data);
        assert_int_equal(expected_data_second,
                         *(size_t *)tree->root->right->data);
        assert_null(affected_leaf);
}

static void test_tree_remove_empty_tree(void **state)
{
        struct tree *tree = *(struct tree **)state;
        struct leaf *affected_leaf = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         tree_remove(tree, tree->root, NULL, &affected_leaf));
        assert_non_null(tree->root);
        assert_null(tree->root->data);
        assert_null(tree->root->left);
        assert_null(tree->root->right);
        assert_null(affected_leaf);
}

static void test_tree_remove_invalid_leaf(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        size_t expected_data_third = 3;
        struct leaf *affected_leaf = NULL;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(
                NO_ERROR,
                tree_insert(tree, tree->root->left, &expected_data_third));
        assert_int_equal(
                INVALID_INPUT_ERROR,
                tree_remove(
                        tree, tree->root->right->left, NULL, &affected_leaf));
        assert_int_equal(3, tree->size);
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->left->data);
        assert_int_equal(expected_data_third,
                         *(size_t *)tree->root->left->right->data);
        assert_int_equal(expected_data_second,
                         *(size_t *)tree->root->right->data);
        assert_null(affected_leaf);
}

static void test_tree_remove_non_primitive(void **state)
{
        // This test case will not be initialized with the setup functions
        UNUSED_FUNCTION_PARAM(state);

        struct tree *tree = tree_create(NULL);

        assert_non_null(tree);

        struct non_primitive_type *expected_data = create_npt("Data", 14);
        struct leaf *affected_leaf = NULL;

        assert_non_null(expected_data);
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, expected_data));

        expect_function_calls(non_primitive_type_free_data, 1);

        assert_int_equal(NO_ERROR,
                         tree_remove(tree,
                                     tree->root,
                                     non_primitive_type_free_data,
                                     &affected_leaf));
        assert_ptr_equal(affected_leaf, tree->root);

        expect_function_calls(non_primitive_leaf_free, 1);

        tree_destroy(tree, non_primitive_leaf_free);
}

static void test_tree_remove_non_primitive_multiple(void **state)
{
        // This test case will not be iniitialized with the setup functions
        UNUSED_FUNCTION_PARAM(state);

        struct tree *tree = tree_create(NULL);

        assert_non_null(tree);

        struct non_primitive_type *expected_data_first = create_npt("One", 1);
        struct non_primitive_type *expected_data_second = create_npt("Two", 2);
        struct non_primitive_type *expected_data_third = create_npt("Three", 3);
        struct leaf *affected_leaf = NULL;

        assert_non_null(expected_data_first);
        assert_non_null(expected_data_second);
        assert_non_null(expected_data_third);
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, expected_data_second));
        assert_int_equal(
                NO_ERROR,
                tree_insert(tree, tree->root->left, expected_data_third));
        assert_int_equal(3, tree->size);

        expect_function_calls(non_primitive_type_free_data, 1);

        assert_int_equal(NO_ERROR,
                         tree_remove(tree,
                                     tree->root->left->right,
                                     non_primitive_type_free_data,
                                     &affected_leaf));

        assert_int_equal(2, tree->size);
        assert_ptr_equal(affected_leaf, tree->root->left);

        // tree->root, tree->root->left, tree->root->right = 3
        expect_function_calls(non_primitive_leaf_free, 3);

        tree_destroy(tree, non_primitive_leaf_free);
}

static void test_tree_comparison_iterate(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data_first = 1;
        size_t data_second = 2;
        size_t expected_result = 3;
        struct leaf *result = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data_first));
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data_second));
        assert_non_null(tree->root->left);
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, tree->root->left, &expected_result));
        assert_non_null(tree->root->left->right);

        // root->right, root->left->left = 2
        expect_function_calls(primitive_leaf_compare, 2);

        assert_int_equal(NO_ERROR,
                         tree_comparison_iterate(tree,
                                                 NULL,
                                                 &expected_result,
                                                 primitive_leaf_compare,
                                                 &result));
        assert_non_null(result);
        assert_int_equal(expected_result, *(size_t *)result->data);
}

static void test_tree_comparison_iterate_find_root(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_result = 14;
        struct leaf *result = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &expected_result));
        assert_non_null(tree->root);

        expect_function_calls(primitive_leaf_compare, 1);

        assert_int_equal(NO_ERROR,
                         tree_comparison_iterate(tree,
                                                 NULL,
                                                 &expected_result,
                                                 primitive_leaf_compare,
                                                 &result));
        assert_non_null(result);
        assert_int_equal(expected_result, *(size_t *)result->data);
}

static void test_tree_comparison_iterate_non_primitive(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        struct tree *tree = tree_create(NULL);

        assert_non_null(tree);

        struct non_primitive_type *data_first = create_npt("one", 1);
        struct non_primitive_type *data_second = create_npt("two", 2);
        struct non_primitive_type *expected_data = create_npt("three", 3);
        struct leaf *result = NULL;

        assert_non_null(data_first);
        assert_non_null(data_second);
        assert_non_null(expected_data);
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, data_first));
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, data_second));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, tree->root->left, expected_data));

        // tree->right tree->left->left = 2
        expect_function_calls(non_primitive_type_compare, 2);

        assert_int_equal(NO_ERROR,
                         tree_comparison_iterate(tree,
                                                 NULL,
                                                 expected_data,
                                                 non_primitive_type_compare,
                                                 &result));
        assert_non_null(result);
        assert_string_equal(
                expected_data->string,
                ((const struct non_primitive_type *)result->data)->string);
        assert_int_equal(
                expected_data->number,
                ((const struct non_primitive_type *)result->data)->number);

        // tree->root, tree->root->left, tree->root->right,
        // tree->root->left->left, tree->root->left->right = 5
        expect_function_calls(non_primitive_leaf_free, 5);

        tree_destroy(tree, non_primitive_leaf_free);
}

static void test_tree_comparison_iterate_not_found(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data_first = 1;
        size_t data_second = 2;
        size_t hidden_data = 3;
        struct leaf *result = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data_first));
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data_second));

        // root->left, root->right = 2
        expect_function_calls(primitive_leaf_compare, 2);

        assert_int_equal(NOT_FOUND_ERROR,
                         tree_comparison_iterate(tree,
                                                 NULL,
                                                 &hidden_data,
                                                 primitive_leaf_compare,
                                                 &result));

        assert_null(result);
}

static void test_tree_comparison_iterate_empty_tree(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t hidden_data = 14;
        struct leaf *result = NULL;

        assert_int_equal(NOT_FOUND_ERROR,
                         tree_comparison_iterate(tree,
                                                 NULL,
                                                 &hidden_data,
                                                 primitive_leaf_compare,
                                                 &result));
        assert_null(result);
}

static void test_leaf_find_parent(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        struct leaf *parent = NULL;

        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_first));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, NULL, &expected_data_second));
        assert_int_equal(2, tree->size);
        assert_non_null(tree->root); // The parent we want
        assert_non_null(tree->root->left); // Going to use this to find it
        assert_int_equal(expected_data_first,
                         *(size_t *)tree->root->left->data);
        assert_int_equal(NO_ERROR, leaf_find_parent(tree->root->left, &parent));
        assert_non_null(parent);
        assert_ptr_equal(parent, tree->root);
        assert_int_equal(expected_data_first, *(size_t *)parent->left->data);
}

static void test_leaf_find_parent_root_single(void **state)
{
        struct tree *tree = *(struct tree **)state;
        struct leaf *parent = NULL;
        size_t data = 14;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(NOT_FOUND_ERROR,
                         leaf_find_parent(tree->root, &parent));
        assert_null(parent);
}

static void test_leaf_find_parent_null(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data = 14;
        struct leaf *parent = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(INVALID_INPUT_ERROR, leaf_find_parent(NULL, &parent));
        assert_null(parent);
}

static void test_leaf_find_sibling(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data_first = 1;
        size_t data_second = 2;
        size_t data_third = 3;
        struct leaf *sibling = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data_first));
        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data_second));
        assert_int_equal(NO_ERROR,
                         tree_insert(tree, tree->root->left, &data_third));
        assert_int_equal(3, tree->size);
        assert_int_equal(NO_ERROR,
                         leaf_find_sibling(tree->root->right, &sibling));
        assert_ptr_equal(sibling, tree->root->right);
}

static void test_leaf_find_sibling_root(void **state)
{
        struct tree *tree = *(struct tree **)state;
        size_t data = 14;
        struct leaf *sibling = NULL;

        assert_int_equal(NO_ERROR, tree_insert(tree, NULL, &data));
        assert_int_equal(NOT_FOUND_ERROR,
                         leaf_find_sibling(tree->root, &sibling));
        assert_null(sibling);
}

static void test_leaf_find_sibling_null(void **state)
{
        // Will not be initialized with test setup
        UNUSED_FUNCTION_PARAM(state);

        struct leaf *sibling = NULL;

        assert_int_equal(INVALID_INPUT_ERROR,
                         leaf_find_sibling(NULL, &sibling));
        assert_null(sibling);
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
                        test_tree_remove, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(test_tree_remove_siblingless,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(test_tree_remove_childless_root,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(test_tree_remove_parent_root,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_remove_empty_tree, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(test_tree_remove_invalid_leaf,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test(test_tree_remove_non_primitive),
                cmocka_unit_test(test_tree_remove_non_primitive_multiple),
                cmocka_unit_test_setup_teardown(test_tree_comparison_iterate,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_comparison_iterate_find_root,
                        test_setup,
                        test_teardown),
                cmocka_unit_test(test_tree_comparison_iterate_non_primitive),
                cmocka_unit_test_setup_teardown(
                        test_tree_comparison_iterate_not_found,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_tree_comparison_iterate_empty_tree,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_leaf_find_parent, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_leaf_find_parent_root_single,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_leaf_find_parent_null, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_leaf_find_sibling, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_leaf_find_sibling_root, test_setup, test_teardown),
                cmocka_unit_test(test_leaf_find_sibling_null),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
