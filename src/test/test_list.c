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
        assert_int_equal(expected_data, *(int *)node->data);

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

static void test_list_insert_after_single_tail(void **state)
{
        int expected_existing_data = 10;
        int expected_created_data = 20;
        struct list *list = *(struct list **)state;

        struct node *existing_node = list_insert(list, &expected_existing_data);

        assert_int_equal(1, list->size);
        assert_int_equal(expected_existing_data, *(int *)list->tail->data);

        struct node *new_node
                = list_insert_after(list, list->tail, &expected_created_data);

        assert_non_null(new_node);

        // Expect head to be the existing node
        assert_memory_equal(
                &expected_existing_data, list->head->data, sizeof(int *));

        // Expect the new node to be after the existing node
        assert_memory_equal(
                existing_node, new_node->previous, sizeof(struct node *));

        // Expect tail to be the newly created data
        assert_memory_equal(
                &expected_created_data, list->tail->data, sizeof(int *));
}

static void test_list_insert_after_single_head(void **state)
{
        int expected_existing_data = 10;
        int expected_created_data = 20;
        struct list *list = *(struct list **)state;

        struct node *existing_node = list_insert(list, &expected_existing_data);

        assert_non_null(existing_node);

        struct node *new_node
                = list_insert_after(list, list->head, &expected_existing_data);

        // Expect head to be existing created data
        assert_memory_equal(
                &expected_existing_data, list->head->data, sizeof(int *));
}

static void test_list_insert_after_middle_node(void **state)
{
        int expected_tail_data = 10;
        int expected_head_data = 20;
        int expected_created_data = 30;
        struct list *list = *(struct list **)state;

        list_insert(list, &expected_tail_data);
        struct node *head_node = list_insert(list, &expected_head_data);

        // When inserting data after the head in a list with this structure:
        // A - C
        // Should result in:
        // A - B - C
        // Where 'B' is the inserted data
        list_insert_after(list, head_node, &expected_created_data);

        assert_memory_equal(
                &expected_tail_data, list->tail->data, sizeof(int *));
        assert_memory_equal(
                &expected_head_data, list->head->data, sizeof(int *));
        assert_memory_equal(
                &expected_created_data, list->head->next->data, sizeof(int *));
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
                        test_list_insert_after_single_head,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_after_single_tail,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_insert_after_middle_node,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_list_empty_succeeds, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}

