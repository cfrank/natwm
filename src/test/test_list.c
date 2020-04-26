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
#include <common/list.h>

static int test_setup(void **state)
{
        struct list *list = list_create();

        if (list == NULL) {
                return EXIT_FAILURE;
        }

        *state = list;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        list_destroy(*(struct list **)state);

        return EXIT_SUCCESS;
}

static void test_node_creation_succeeds(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        int expected_data = 10;
        struct node *node = node_create(&expected_data);

        assert_non_null(node);
        assert_null(node->next);
        assert_null(node->previous);
        assert_int_equal(expected_data, *(const int *)node->data);

        node_destroy(node);
}

static void test_list_creation_succeeds(void **state)
{
        // create_list is called in the setup function
        struct list *list = *(struct list **)state;

        assert_null(list->head);
        assert_null(list->tail);
        assert_int_equal(0, list->size);
}

static void test_list_insert_after_single_node(void **state)
{
        struct list *list = *(struct list **)state;
        struct node *existing_node = list_insert(list, NULL);

        assert_int_equal(1, list->size);
        assert_memory_equal(existing_node, list->tail, sizeof(struct node *));

        struct node *new_node = list_insert_after(list, list->tail, NULL);

        assert_non_null(new_node);

        // Expect head to be the existing node
        assert_memory_equal(existing_node, list->head, sizeof(struct node *));

        // Expect the new node to be after the existing node
        assert_memory_equal(
                new_node, existing_node->next, sizeof(struct node *));

        // Expect tail to be the newly created data
        assert_memory_equal(new_node, list->tail, sizeof(struct node *));
}

static void test_list_insert_after_middle_node(void **state)
{
        struct list *list = *(struct list **)state;
        struct node *tail_node = list_insert(list, NULL); // 'D'
        struct node *middle_node = list_insert(list, NULL); // 'C'
        struct node *head_node = list_insert(list, NULL); // 'A'

        // When inserting data after 'C' in a list with this structure:
        // D - C - A
        // Should result in:
        // D - C - B - A
        // Where 'B' is the inserted data
        struct node *created_node = list_insert_after(list, head_node, NULL);

        // Head and tail should be the same
        assert_memory_equal(tail_node, list->tail, sizeof(struct node *));
        assert_memory_equal(head_node, list->head, sizeof(struct node *));

        // Created node should be after middle node
        assert_memory_equal(
                middle_node, created_node->next, sizeof(struct node *));
}

static void test_list_insert_before_single_node(void **state)
{
        struct list *list = *(struct list **)state;
        struct node *existing_node = list_insert(list, NULL);

        assert_memory_equal(existing_node, list->head, sizeof(struct node *));

        struct node *new_node = list_insert_before(list, existing_node, NULL);

        assert_int_equal(2, list->size);

        // Expect tail to be the existing node
        assert_memory_equal(existing_node, list->tail, sizeof(struct node *));

        // Expect the new node to be before the existing node
        assert_memory_equal(
                new_node, existing_node->previous, sizeof(struct node *));

        // Expect the list head to be the new node
        assert_memory_equal(new_node, list->head, sizeof(struct node *));
}

static void test_list_insert_before_middle_node(void **state)
{
        struct list *list = *(struct list **)state;
        struct node *tail_node = list_insert(list, NULL); // 'D'
        struct node *middle_node = list_insert(list, NULL); // 'C'
        struct node *head_node = list_insert(list, NULL); // 'A'

        // When inserting a node before 'C' in a list with this
        // structure 'D' - 'C' - 'A' The resulting list should look like
        // this 'D' - 'C' - 'B' - 'A'

        struct node *created_node = list_insert_before(list, middle_node, NULL);

        assert_int_equal(4, list->size);

        // Expect the head and tail to be the same
        assert_memory_equal(head_node, list->head, sizeof(struct node *));
        assert_memory_equal(tail_node, list->tail, sizeof(struct node *));

        // Expect the created node to come before the middle node
        assert_memory_equal(
                created_node, middle_node->previous, sizeof(struct node *));

        // Expect the middle node to come after the created node
        assert_memory_equal(
                middle_node, created_node->next, sizeof(struct node *));
}

static void test_list_insert_empty_list(void **state)
{
        struct list *list = *(struct list **)state;

        assert_int_equal(0, list->size);

        struct node *node = list_insert(list, NULL);

        // Expect the list to have one element and the head and tail to
        // be the inserted node
        assert_int_equal(1, list->size);
        assert_memory_equal(node, list->head, sizeof(struct node *));
        assert_memory_equal(node, list->tail, sizeof(struct node *));
}

static void test_list_insert_occupied_list(void **state)
{
        size_t num_nodes = 5;
        struct list *list = *(struct list **)state;

        for (size_t i = 0; i < num_nodes; ++i) {
                list_insert(list, NULL);
        }

        assert_int_equal(num_nodes, list->size);

        // When inserting a node in a list which already contains data that
        // newly inserted node should occupy the head of the list

        struct node *new_node = list_insert(list, NULL);

        assert_int_equal((num_nodes + 1), list->size);
        assert_memory_equal(new_node, list->head, sizeof(struct node *));
}

static void test_list_insert_end_empty_list(void **state)
{
        struct list *list = *(struct list **)state;

        assert_int_equal(0, list->size);

        struct node *node = list_insert_end(list, NULL);

        // When doing a list_insert_end call on a list with no data it should
        // behave exactly like list_insert. Thus the data should occupy the head
        // and tail of the list
        assert_int_equal(1, list->size);
        assert_memory_equal(node, list->head, sizeof(struct node *));
        assert_memory_equal(node, list->tail, sizeof(struct node *));
}

static void test_list_insert_end_occupied_list(void **state)
{
        struct list *list = *(struct list **)state;

        struct node *old_tail_node = list_insert(list, NULL);
        struct node *head_node = list_insert(list, NULL);

        assert_int_equal(2, list->size);

        struct node *new_node = list_insert_end(list, NULL);

        // When inserting a node into the end of a list which already contains
        // data it should occupy the tail and the old tail should come directly
        // after it. The head of the list should stay the same
        assert_int_equal(3, list->size);
        assert_memory_equal(new_node, list->tail, sizeof(struct node *));
        assert_memory_equal(
                old_tail_node, list->tail->previous, sizeof(struct node *));
        assert_memory_equal(head_node, list->head, sizeof(struct node *));
}

static void test_list_move_node_to_head(void **state)
{
        struct list *list = *(struct list **)state;

        // Expected data for the node which will be moved
        size_t expected_data = 14;
        // Expected data for the node which will be the existing head
        size_t expected_data_next = 41;
        struct node *expected_node = list_insert(list, &expected_data);

        assert_non_null(expected_node);

        assert_non_null(list_insert(list, NULL));
        assert_non_null(list_insert(list, NULL));
        assert_non_null(list_insert(list, NULL));
        assert_non_null(list_insert(list, NULL));

        struct node *expected_node_next
                = list_insert(list, &expected_data_next);

        assert_false(list_is_empty(list));
        assert_int_equal(6, list->size);

        assert_int_equal(expected_data, *(size_t *)list->tail->data);

        assert_non_null(
                list_insert_node_after(list, list->tail, node_create(NULL)));

        assert_memory_not_equal(
                expected_node, list->tail, sizeof(struct node *));

        assert_int_equal(expected_data_next, *(size_t *)list->head->data);

        list_move_node_to_head(list, expected_node);

        assert_memory_equal(expected_node, list->head, sizeof(struct node *));
        assert_memory_equal(
                expected_node_next, expected_node->next, sizeof(struct node *));
        assert_int_equal(expected_data, *(size_t *)list->head->data);
}

static void test_list_move_node_to_tail(void **state)
{
        struct list *list = *(struct list **)state;

        size_t expected_data = 14;
        size_t expected_previous = 28;
        // Create what will become the "old" tail
        struct node *expected_previous_node
                = list_insert(list, &expected_previous);

        // Add some additional nodes
        assert_non_null(list_insert(list, NULL));
        assert_non_null(list_insert(list, NULL));
        assert_non_null(list_insert(list, NULL));
        assert_non_null(list_insert(list, NULL));

        // Asserts to make sure the expected "old" tail is currently the tail
        assert_non_null(expected_previous_node);
        assert_memory_equal(
                expected_previous_node, list->tail, sizeof(struct node *));
        assert_int_equal(expected_previous, *(size_t *)list->tail->data);

        // Add what will become to the "new" tail
        struct node *expected_node = list_insert(list, &expected_data);

        // Asserts to make sure the expected "new" tail is currently the head
        assert_non_null(expected_node);
        assert_memory_equal(expected_node, list->head, sizeof(struct node *));
        assert_int_equal(expected_data, *(size_t *)list->head->data);

        // Move head to tail
        list_move_node_to_tail(list, list->head);

        // Asserts to make sure the "new" tail is moved from head to tail
        assert_memory_equal(expected_node, list->tail, sizeof(struct node *));
        assert_int_equal(expected_data, *(size_t *)list->tail->data);
        assert_memory_equal(expected_previous_node,
                            list->tail->previous,
                            sizeof(struct node *));
        assert_memory_not_equal(
                expected_node, list->head, sizeof(struct node *));
}

static void test_list_remove_head_node(void **state)
{
        struct list *list = *(struct list **)state;

        struct node *node = list_insert(list, NULL);

        assert_int_equal(1, list->size);

        list_remove(list, node);
        node_destroy(node);

        assert_int_equal(0, list->size);
        assert_null(list->head);
        assert_null(list->tail);
}

static void test_list_remove_tail_node(void **state)
{
        struct list *list = *(struct list **)state;
        struct node *tail_node = list_insert(list, NULL);

        // Insert a node which will be after the tail node
        struct node *new_node = list_insert(list, NULL);

        assert_int_equal(2, list->size);
        assert_memory_equal(tail_node, list->tail, sizeof(struct node *));

        // Get rid of the tail
        list_remove(list, tail_node);
        node_destroy(tail_node);

        assert_int_equal(1, list->size);

        // Make sure the tail is now the new node
        assert_memory_equal(new_node, list->tail, sizeof(struct node *));

        // The new node should still be the head
        assert_memory_equal(new_node, list->head, sizeof(struct node *));
}

static void test_list_remove_middle_node(void **state)
{
        struct list *list = *(struct list **)state;

        struct node *tail_node = list_insert(list, NULL);
        struct node *middle_node = list_insert(list, NULL);
        struct node *head_node = list_insert(list, NULL);

        assert_int_equal(3, list->size);
        assert_memory_equal(
                middle_node, list->head->next, sizeof(struct node *));

        list_remove(list, middle_node);
        node_destroy(middle_node);

        // Removing the middle node should leave the head and tail the
        // same
        assert_int_equal(2, list->size);
        assert_memory_equal(head_node, list->head, sizeof(struct node *));
        assert_memory_equal(tail_node, list->tail, sizeof(struct node *));

        // And result in the next node after the head to be the tail
        assert_memory_equal(tail_node, list->head->next, sizeof(struct node *));
}

static void test_list_is_empty_succeeds(void **state)
{
        struct list *list = *(struct list **)state;

        assert_true(list_is_empty(list));

        list_insert(list, NULL);

        assert_false(list_is_empty(list));
}

static void test_list_empty_succeeds(void **state)
{
        int value = 10;
        struct list *list = *(struct list **)state;

        struct node *first = list_insert(list, &value);
        struct node *second = list_insert(list, &value);
        struct node *third = list_insert(list, &value);

        assert_int_equal(3, list->size);

        list_empty(list);

        assert_int_equal(0, list->size);
        assert_true(list_is_empty(list));

        node_destroy(first);
        node_destroy(second);
        node_destroy(third);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_node_creation_succeeds, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_creation_succeeds, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_after_single_node,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_after_middle_node,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_before_single_node,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_before_middle_node,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_empty_list, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(test_list_insert_occupied_list,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(test_list_insert_end_empty_list,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_end_occupied_list,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_move_node_to_head, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_move_node_to_tail, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_remove_head_node, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_remove_tail_node, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(test_list_remove_middle_node,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_is_empty_succeeds, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_empty_succeeds, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
