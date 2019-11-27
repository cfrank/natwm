// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "error.h"

struct stack_item {
        struct stack_item *next;
        const void *data;
};

struct stack {
        size_t length;
        struct stack_item *head;
};

struct stack *stack_create(void);
struct stack_item *stack_item_create(void *data);

bool stack_has_item(struct stack *stack);

void stack_push_item(struct stack *stack, struct stack_item *item);
enum natwm_error stack_push(struct stack *stack, void *data);
void stack_enqueue_item(struct stack *stack, struct stack_item *item);
enum natwm_error stack_enqueue(struct stack *stack, void *data);

struct stack_item *stack_pop(struct stack *stack);
struct stack_item *stack_dequeue(struct stack *state);

void stack_item_destroy(struct stack_item *item);
void stack_destroy(struct stack *state);
