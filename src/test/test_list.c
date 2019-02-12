#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <core/list.h>

static void test_list_creation_succeeds(void **state)
{
        struct list *list = create_list();

        assert_non_null(list);
        assert_null(list->head);
        assert_null(list->tail);
        assert_int_equal(0, list->size);
}

static void test_node_creation_succeeds(void **state)
{
        int expected_data = 10;
        struct node *node = create_node(&expected_data);

        assert_non_null(node);
        assert_null(node->next);
        assert_null(node->previous);
        assert_int_equal(expected_data, *(int *)node->data);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_list_creation_succeeds),
                cmocka_unit_test(test_node_creation_succeeds),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}

