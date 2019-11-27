// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include "stack.h"

struct stack *stack_create(void)
{
        struct stack *stack = malloc(sizeof(struct stack));

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

bool stack_has_item(struct stack *stack)
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

                return;
        }

        // Need to find last item
        struct stack_item *curr = stack->head;

        while (curr != NULL) {
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

struct stack_item *stack_dequeue(struct stack *stack)
{
        if (!stack_has_item(stack)) {
                return NULL;
        }

        struct stack_item *curr = stack->head;
        struct stack_item *prev = NULL;

        while (curr != NULL) {
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

void stack_destroy(struct stack *stack)
{
        free(stack);
}

