#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <core/list.h>

static void test_list_creation_succeeds(void **state)
{
        struct list *list = create_list();

        assert_null(list->head);
        assert_null(list->tail);
        assert_int_equal(0, list->size);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test_list_creation_succeeds),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}

