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

// Set up for *_destroy_callback tests

struct non_primitive_type {
        const char *string;
        size_t number;
};

struct non_primitive_type *npt_create(const char *string, size_t number)
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

static void npt_destroy(struct non_primitive_type *npt)
{
        function_called();

        if (npt == NULL) {
                return;
        }

        free(npt);
}

static void stack_data_destroy_callback(void *data)
{
        function_called();

        struct non_primitive_type *npt = (struct non_primitive_type *)data;

        npt_destroy(npt);
}

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

static void test_stack_peek(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data = 14;

        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data));
        assert_true(stack_has_item(stack));

        const struct stack_item *stack_item = stack_peek(stack);

        assert_non_null(stack_item);
        assert_int_equal(expected_data, *(size_t *)stack_item->data);
        assert_true(stack_has_item(stack));
        assert_int_equal(1, stack->length);
}

static void test_stack_peek_multiple(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t first = 123;
        size_t expected_data = 123;

        assert_int_equal(NO_ERROR, stack_push(stack, &first));
        assert_true(stack_has_item(stack));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data));
        assert_int_equal(2, stack->length);

        const struct stack_item *stack_item = stack_peek(stack);

        assert_non_null(stack_item);
        assert_int_equal(expected_data, *(size_t *)stack_item->data);
        assert_true(stack_has_item(stack));
        assert_int_equal(2, stack->length);
}

static void test_stack_peek_empty(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t data = 14;

        assert_false(stack_has_item(stack));
        assert_null(stack_peek(stack));
        assert_int_equal(NO_ERROR, stack_push(stack, &data));
        assert_true(stack_has_item(stack));
        assert_int_equal(1, stack->length);

        struct stack_item *stack_item = stack_pop(stack);

        assert_non_null(stack_item);
        assert_false(stack_has_item(stack));
        assert_null(stack_peek(stack));

        stack_item_destroy(stack_item);
}

static void test_stack_peek_n(void **state)
{
        struct stack *stack = *(struct stack **)state;
        size_t expected_data_first = 123;
        size_t expected_data_second = 456;
        size_t expected_data_third = 456;

        assert_false(stack_has_item(stack));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_first));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_second));
        assert_int_equal(NO_ERROR, stack_push(stack, &expected_data_third));
        assert_true(stack_has_item(stack));
        assert_int_equal(3, stack->length);

        const struct stack_item *stack_item_zero = stack_peek_n(stack, 0);
        const struct stack_item *stack_item_one = stack_peek_n(stack, 1);
        const struct stack_item *stack_item_two = stack_peek_n(stack, 2);

        assert_non_null(stack_item_zero);
        assert_non_null(stack_item_one);
        assert_non_null(stack_item_two);
        assert_true(stack_has_item(stack));
        assert_int_equal(3, stack->length);
        assert_int_equal(expected_data_third, *(size_t *)stack_item_zero->data);
        assert_int_equal(expected_data_second, *(size_t *)stack_item_one->data);
        assert_int_equal(expected_data_first, *(size_t *)stack_item_two->data);
}

static void test_stack_peek_n_not_found(void **state)
{
        struct stack *stack = *(struct stack **)state;

        assert_false(stack_has_item(stack));
        assert_null(stack_peek_n(stack, 0));
        assert_null(stack_peek_n(stack, 5));
        assert_null(stack_peek_n(stack, 10));
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

static void test_stack_destroy_callback(void **state)
{
        // Need to create our own NPT stack+items
        UNUSED_FUNCTION_PARAM(state);

        struct stack *stack = stack_create();

        assert_non_null(stack);

        struct non_primitive_type *first = npt_create("Test", 14);

        assert_int_equal(NO_ERROR, stack_push(stack, first));

        assert_int_equal(1, stack->length);
        assert_non_null(stack->head);

        expect_function_call(stack_data_destroy_callback);
        expect_function_call(npt_destroy);

        stack_destroy_callback(stack, stack_data_destroy_callback);
}

static void test_stack_destroy_callback_multiple(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        struct stack *stack = stack_create();

        assert_non_null(stack);

        const char *expected_string = "Head item";
        struct non_primitive_type *first = npt_create("Tail Item", 1);
        struct non_primitive_type *second = npt_create(expected_string, 2);

        assert_int_equal(NO_ERROR, stack_push(stack, first));
        assert_int_equal(NO_ERROR, stack_push(stack, second));

        assert_int_equal(2, stack->length);
        assert_non_null(stack->head->next);

        expect_function_call(stack_data_destroy_callback);
        expect_function_call(npt_destroy);

        struct stack_item *item_head = stack_pop(stack);

        assert_non_null(item_head);

        stack_item_destroy_callback(item_head, stack_data_destroy_callback);

        assert_int_equal(1, stack->length);
        assert_null(stack->head->next);

        expect_function_call(stack_data_destroy_callback);
        expect_function_call(npt_destroy);

        stack_destroy_callback(stack, stack_data_destroy_callback);
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
                        test_stack_peek, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_peek_multiple, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_peek_empty, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_peek_n, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_peek_n_not_found, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_dequeue, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_dequeue_multiple, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_stack_dequeue_empty, test_setup, test_teardown),
                cmocka_unit_test(test_stack_destroy_callback),
                cmocka_unit_test(test_stack_destroy_callback_multiple),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
