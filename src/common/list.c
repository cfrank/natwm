// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stddef.h>
#include <stdlib.h>

#include "list.h"

static void clear_list(struct list *list, bool destroy)
{
        struct node *node = list->head;
        struct node *next = NULL;

        while (node != NULL) {
                next = node->next;

                list_remove(list, node);

                if (destroy) {
                        node_destroy(node);
                }

                node = next;
        }
}

struct node *node_create(const void *data)
{
        struct node *node = malloc(sizeof(struct node));

        if (node == NULL) {
                return NULL;
        }

        node->next = NULL;
        node->previous = NULL;
        node->data = data;

        return node;
}

struct list *list_create(void)
{
        struct list *list = malloc(sizeof(struct list));

        if (list == NULL) {
                return NULL;
        }

        list->head = NULL;
        list->tail = NULL;
        list->size = 0;

        return list;
}

struct node *list_insert_node_after(struct list *list, struct node *existing,
                                    struct node *new)
{
        if (list == NULL || existing == NULL || new == NULL) {
                return NULL;
        }

        new->previous = existing;

        if (existing->next == NULL) {
                list->tail = new;
        } else {
                new->next = existing->next;
                existing->next->previous = new;
        }

        existing->next = new;

        ++list->size;

        return new;
}

struct node *list_insert_after(struct list *list, struct node *node,
                               const void *data)
{
        return list_insert_node_after(list, node, node_create(data));
}

struct node *list_insert_node_before(struct list *list, struct node *existing,
                                     struct node *new)
{
        if (list == NULL || existing == NULL || new == NULL) {
                return NULL;
        }

        new->next = existing;

        if (existing->previous == NULL) {
                // If the existing node is the head then we need to make the
                // new node the new head
                list->head = new;
        } else {
                new->previous = existing;
                existing->previous->next = new;
        }

        existing->previous = new;

        ++list->size;

        return new;
}

struct node *list_insert_before(struct list *list, struct node *node,
                                const void *data)
{
        return list_insert_node_before(list, node, node_create(data));
}

struct node *list_insert_node(struct list *list, struct node *node)
{
        if (list == NULL || node == NULL) {
                return NULL;
        }

        if (list->head == NULL) {
                list->head = node;
                list->tail = node;

                ++list->size;

                return node;
        } else {
                return list_insert_node_before(list, list->head, node);
        }
}

struct node *list_insert(struct list *list, const void *data)
{
        return list_insert_node(list, node_create(data));
}

struct node *list_insert_end(struct list *list, const void *data)
{
        if (list->tail == NULL) {
                return list_insert(list, data);
        } else {
                return list_insert_after(list, list->tail, data);
        }
}

void list_move_node_to_head(struct list *list, struct node *node)
{
        if (node->previous == NULL) {
                return;
        }

        list_remove(list, node);

        list_insert_node(list, node);
}

void list_move_node_to_tail(struct list *list, struct node *node)
{
        if (node->next == NULL) {
                return;
        }

        list_remove(list, node);

        list_insert_node_after(list, list->tail, node);
}

void list_remove(struct list *list, struct node *node)
{
        if (node->previous == NULL) {
                // First element
                list->head = node->next;
        } else {
                node->previous->next = node->next;
        }

        if (node->next == NULL) {
                // Last element
                list->tail = node->previous;
        } else {
                node->next->previous = node->previous;
        }

        node->next = NULL;
        node->previous = NULL;

        --list->size;
}

bool list_is_empty(const struct list *list)
{
        return list->size == 0;
}

void node_destroy(struct node *node)
{
        free(node);
}

void list_destroy(struct list *list)
{
        clear_list(list, true);

        free(list);
}

void list_empty(struct list *list)
{
        clear_list(list, false);
}
