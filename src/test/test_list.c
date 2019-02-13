#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>
#include <core/list.h>

static int test_setup(void **state)
{
        struct list *list = create_list();

        if (list == NULL) {
                return EXIT_FAILURE;
        }

        *state = list;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        destroy_list(*(struct list **)state);

        return EXIT_SUCCESS;
}

static void test_node_creation_succeeds(void **state)
{
        int expected_data = 10;
        struct node *node = create_node(&expected_data);

        assert_non_null(node);
        assert_null(node->next);
        assert_null(node->previous);
        assert_int_equal(expected_data, *(const int *)node->data);

        destroy_node(node);
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

static void test_list_remove_head_node(void **state)
{
        struct list *list = *(struct list **)state;

        struct node *node = list_insert(list, NULL);

        assert_int_equal(1, list->size);

        list_remove(list, node);

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

        list_insert(list, &value);
        list_insert(list, &value);
        list_insert(list, &value);

        assert_int_equal(3, list->size);

        empty_list(list);

        assert_int_equal(0, list->size);
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

