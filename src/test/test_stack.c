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
#include <common/stack.h>

static int test_setup(void **state)
{
        struct stack *stack = stack_create();

        if (stack == NULL) {
                return EXIT_FAILURE;
        }

        *state = stack;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        stack_destroy(*(struct stack **)state);

        return EXIT_SUCCESS;
}

static void test_stack_creation(void **state)
{
        struct stack *stack = *(struct stack **)state;

        assert_false(stack_has_item(stack));
        assert_null(stack->head);
}

static void test_stack_item_creation(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        size_t expected_data = 14;

        struct stack_item *item = stack_item_create(&expected_data);

        assert_null(item->next);
        assert_int_equal(expected_data, *(size_t *)item->data);

        free(item);
}

static void test_stack_push_item(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;
        struct stack_item *item = stack_item_create(&expected_data);

        assert_null(stack->head);

        stack_push_item(stack, item);

        assert_non_null(stack->head);
        assert_int_equal(1, stack->length);
        assert_null(stack->head->next);
        assert_int_equal(expected_data, *(size_t *)stack->head->data);
}

static void test_stack_push_item_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        struct stack_item *first = stack_item_create(&expected_data_first);
        struct stack_item *second = stack_item_create(&expected_data_second);

        stack_push_item(stack, first);
        stack_push_item(stack, second);

        assert_true(stack_has_item(stack));

        assert_int_equal(2, stack->length);
        assert_int_equal(expected_data_second, *(size_t *)stack->head->data);
        assert_int_equal(expected_data_first,
                         *(size_t *)stack->head->next->data);
        assert_null(stack->head->next->next);
}

static void test_stack_push(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data));
        assert_int_equal(1, stack->length);
        assert_non_null(stack->head);
        assert_int_equal(expected_data, *(size_t *)stack->head->data);
        assert_null(stack->head->next);
}

static void test_stack_push_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_first));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_second));

        assert_int_equal(2, stack->length);
        assert_non_null(stack->head);
        assert_non_null(stack->head->next);
        assert_int_equal(expected_data_second, *(size_t *)stack->head->data);
        assert_int_equal(expected_data_first,
                         *(size_t *)stack->head->next->data);
}

static void test_stack_enqueue_item(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;
        struct stack_item *item = stack_item_create(&expected_data);

        stack_enqueue_item(stack, item);

        assert_int_equal(1, stack->length);
        assert_non_null(stack->head);
        assert_int_equal(expected_data, *((size_t *)stack->head->data));
        assert_null(stack->head->next);
}

static void test_stack_enqueue_item_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;
        struct stack_item *item_first = stack_item_create(&expected_data_first);
        struct stack_item *item_second
                = stack_item_create(&expected_data_second);

        stack_enqueue_item(stack, item_first);
        stack_enqueue_item(stack, item_second);

        assert_int_equal(2, stack->length);
        assert_non_null(stack->head);
        assert_non_null(stack->head->next);
        assert_int_equal(expected_data_first, *(size_t *)stack->head->data);
        assert_int_equal(expected_data_second,
                         *(size_t *)stack->head->next->data);
}

static void test_stack_enqueue(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;

        assert_int_equal(NO_ERROR, stack_enqueue(stack, &expected_data));
        assert_int_equal(1, stack->length);
        assert_non_null(stack->head);
        assert_int_equal(expected_data, *(size_t *)stack->head->data);
}

static void test_stack_enqueue_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;

        assert_int_equal(NO_ERROR, stack_enqueue(stack, &expected_data_first));
        assert_int_equal(NO_ERROR, stack_enqueue(stack, &expected_data_second));
        assert_int_equal(2, stack->length);
        assert_non_null(stack->head);
        assert_non_null(stack->head->next);
        assert_int_equal(expected_data_first, *(size_t *)stack->head->data);
        assert_int_equal(expected_data_second,
                         *(size_t *)stack->head->next->data);
}

static void test_stack_pop(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data));
        assert_int_equal(1, stack->length);
        assert_non_null(stack->head);

        struct stack_item *item = stack_pop(stack);

        assert_false(stack_has_item(stack));
        assert_null(stack->head);
        assert_int_equal(expected_data, *(size_t *)item->data);

        stack_item_destroy(item);
}

static void test_stack_pop_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 1;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_first));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_second));

        assert_int_equal(2, stack->length);
        assert_non_null(stack->head->next);

        struct stack_item *item_first = stack_pop(stack);

        assert_null(stack->head->next);
        assert_int_equal(expected_data_first, *(size_t *)stack->head->data);
        assert_ptr_equal(item_first->next, stack->head);

        struct stack_item *item_second = stack_pop(stack);

        assert_false(stack_has_item(stack));
        assert_null(stack->head);
        assert_non_null(item_first);
        assert_non_null(item_first->next);
        assert_non_null(item_second);
        assert_null(item_second->next);
        assert_int_equal(expected_data_second, *(size_t *)item_first->data);
        assert_int_equal(expected_data_first, *(size_t *)item_second->data);

        stack_item_destroy(item_first);
        stack_item_destroy(item_second);
}

static void test_stack_pop_empty(void **state)
{
        struct stack *stack = *(struct stack **)state;

        assert_false(stack_has_item(stack));
        assert_null(stack_pop(stack));
        assert_false(stack_has_item(stack));
}

static void test_stack_dequeue(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data));
        assert_int_equal(1, stack->length);

        struct stack_item *item = stack_dequeue(stack);

        assert_false(stack_has_item(stack));
        assert_non_null(item);
        assert_null(stack->head);
        assert_null(item->next);
        assert_int_equal(expected_data, *(size_t *)item->data);

        stack_item_destroy(item);
}

static void test_stack_dequeue_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 1;
        size_t expected_data_second = 2;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_first));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_second));

        assert_int_equal(2, stack->length);
        assert_non_null(stack->head->next);

        struct stack_item *item_first = stack_dequeue(stack);

        assert_int_equal(1, stack->length);
        assert_null(stack->head->next);
        assert_null(item_first->next);

        struct stack_item *item_second = stack_dequeue(stack);

        assert_false(stack_has_item(stack));
        assert_null(stack->head);
        assert_null(item_second->next);
        assert_int_equal(expected_data_first, *(size_t *)item_first->data);
        assert_int_equal(expected_data_second, *(size_t *)item_second->data);

        stack_item_destroy(item_first);
        stack_item_destroy(item_second);
}

static void test_stack_dequeue_empty(void **state)
{
        struct stack *stack = *(struct stack **)state;

        assert_false(stack_has_item(stack));
        assert_null(stack_dequeue(stack));
        assert_false(stack_has_item(stack));
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_stack_creation, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_item_creation, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_push_item, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(test_stack_push_item_multiple,
                                                test_setup,
                                                test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_push, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_push_multiple, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_enqueue_item, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_enqueue_item_multiple,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_enqueue, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_enqueue_multiple, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_pop, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_pop_multiple, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_pop_empty, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_dequeue, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_dequeue_multiple, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_dequeue_empty, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
