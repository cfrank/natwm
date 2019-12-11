// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>

struct node {
        struct node *next;
        struct node *previous;
        const void *data;
};

struct list {
        struct node *head;
        struct node *tail;
        size_t size;
};

#define LIST_FOR_EACH(list, item)                                              \
        for (struct node * (item) = (list)->head; (item) != NULL;              \
             (item) = (item)->next)

struct node *create_node(const void *data);
void destroy_node(struct node *node);

struct list *create_list(void);
struct node *list_insert_after(struct list *list, struct node *node,
                               const void *data);
struct node *list_insert_before(struct list *list, struct node *node,
                                const void *data);
struct node *list_insert(struct list *list, const void *data);
struct node *list_insert_end(struct list *list, const void *data);
void list_remove(struct list *list, struct node *node);
bool list_is_empty(const struct list *list);
void destroy_list(struct list *list);
void empty_list(struct list *list);
