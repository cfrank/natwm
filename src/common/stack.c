// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include "stack.h"

struct stack *stack_create(void)
{
        struct stack *stack = malloc(sizeof(struct stack));

        if (stack == NULL) {
                return NULL;
        }

        stack->length = 0;
        stack->head = NULL;

        return stack;
}

struct stack_item *stack_item_create(void *data)
{
        struct stack_item *item = malloc(sizeof(struct stack_item));

        if (item == NULL) {
                return NULL;
        }

        item->next = NULL;
        item->data = data;

        return item;
}

bool stack_has_item(const struct stack *stack)
{
        return stack->head != NULL;
}

void stack_push_item(struct stack *stack, struct stack_item *item)
{
        if (stack_has_item(stack)) {
                item->next = stack->head;
        }

        stack->head = item;
        ++stack->length;
}

enum natwm_error stack_push(struct stack *stack, void *data)
{
        struct stack_item *item = stack_item_create(data);

        if (item == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        stack_push_item(stack, item);

        return NO_ERROR;
}

void stack_enqueue_item(struct stack *stack, struct stack_item *item)
{
        if (!stack_has_item(stack)) {
                stack->head = item;
                ++stack->length;

                return;
        }

        // Need to find last item
        struct stack_item *curr = stack->head;

        while (curr != NULL && curr->next != NULL) {
                curr = curr->next;
        }

        // curr should point to the last item now
        curr->next = item;
        ++stack->length;
}

enum natwm_error stack_enqueue(struct stack *stack, void *data)
{
        struct stack_item *item = stack_item_create(data);

        if (item == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        stack_enqueue_item(stack, item);

        return NO_ERROR;
}

struct stack_item *stack_pop(struct stack *stack)
{
        if (!stack_has_item(stack)) {
                return NULL;
        }

        struct stack_item *item = stack->head;

        stack->head = item->next;
        --stack->length;

        return item;
}

const struct stack_item *stack_peek_n(const struct stack *stack, size_t index)
{
        if (!stack_has_item(stack) || stack->length < index) {
                return NULL;
        }

        struct stack_item *curr = stack->head;
        size_t i = 0;

        while (i < index && curr != NULL && curr->next != NULL) {
                curr = curr->next;

                ++i;
        }

        return curr;
}

const struct stack_item *stack_peek(const struct stack *stack)
{
        return stack_peek_n(stack, 0);
}

struct stack_item *stack_dequeue(struct stack *stack)
{
        if (!stack_has_item(stack)) {
                return NULL;
        }

        if (stack->length == 1) {
                return stack_pop(stack);
        }

        struct stack_item *curr = stack->head;
        struct stack_item *prev = NULL;

        while (curr != NULL && curr->next != NULL) {
                prev = curr;
                curr = curr->next;
        }

        // curr should now point to the last item
        prev->next = NULL;
        --stack->length;

        return curr;
}

void stack_item_destroy(struct stack_item *item)
{
        free(item);
}

void stack_item_destroy_callback(struct stack_item *item,
                                 stack_data_free_function free_function)
{
        if (free_function == NULL) {
                stack_item_destroy(item);

                return;
        }

        free_function((void *)item->data);

        stack_item_destroy(item);
}

void stack_destroy(struct stack *stack)
{
        struct stack_item *curr = NULL;

        while ((curr = stack_pop(stack)) != NULL) {
                stack_item_destroy(curr);
        }

        free(stack);
}

void stack_destroy_callback(struct stack *stack,
                            stack_data_free_function free_function)
{
        if (free_function == NULL) {
                stack_destroy(stack);

                return;
        }

        struct stack_item *curr = NULL;

        while ((curr = stack_pop(stack)) != NULL) {
                free_function((void *)curr->data);

                stack_item_destroy(curr);
        }

        free(stack);
}
