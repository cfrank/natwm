// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stddef.h>

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

struct node *node_create(const void *data);
struct list *list_create(void);

struct node *list_insert_node_after(struct list *list, struct node *existing,
                                    struct node *new);
struct node *list_insert_after(struct list *list, struct node *node,
                               const void *data);
struct node *list_insert_node_before(struct list *list, struct node *existing,
                                     struct node *new);
struct node *list_insert_before(struct list *list, struct node *node,
                                const void *data);
struct node *list_insert_node(struct list *list, struct node *node);
struct node *list_insert(struct list *list, const void *data);
struct node *list_insert_end(struct list *list, const void *data);
void list_move_node_to_head(struct list *list, struct node *node);
void list_move_node_to_tail(struct list *list, struct node *node);
void list_remove(struct list *list, struct node *node);
bool list_is_empty(const struct list *list);

void node_destroy(struct node *node);
void list_destroy(struct list *list);
void list_empty(struct list *list);
