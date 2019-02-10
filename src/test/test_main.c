#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

static void test(void **state)
{
        assert_int_equal(1, 1);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test(test),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
