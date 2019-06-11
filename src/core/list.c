// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stddef.h>
#include <stdlib.h>

#include "list.h"

struct node *create_node(const void *data)
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

void destroy_node(struct node *node)
{
        free(node);
}

struct list *create_list(void)
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

struct node *list_insert_after(struct list *list, struct node *node,
                               const void *data)
{
        struct node *new_node = create_node(data);

        if (new_node == NULL) {
                return NULL;
        }

        new_node->previous = node;

        if (node->next == NULL) {
                list->tail = new_node;
        } else {
                new_node->next = node->next;
                node->next->previous = new_node;
        }

        node->next = new_node;
        ++list->size;

        return new_node;
}

struct node *list_insert_before(struct list *list, struct node *node,
                                const void *data)
{
        struct node *new_node = create_node(data);

        if (new_node == NULL) {
                return NULL;
        }

        new_node->next = node;

        if (node->previous == NULL) {
                list->head = new_node;
        } else {
                new_node->previous = node;
                node->previous->next = new_node;
        }

        node->previous = new_node;
        ++list->size;

        return new_node;
}

struct node *list_insert(struct list *list, const void *data)
{
        if (list->head == NULL) {
                struct node *new_node = create_node(data);

                if (new_node == NULL) {
                        return NULL;
                }

                list->head = new_node;
                list->tail = new_node;
                ++list->size;

                return new_node;
        } else {
                return list_insert_before(list, list->head, data);
        }
}

struct node *list_insert_end(struct list *list, const void *data)
{
        if (list->tail == NULL) {
                return list_insert(list, data);
        } else {
                return list_insert_after(list, list->tail, data);
        }
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

        --list->size;
}

bool list_is_empty(const struct list *list)
{
        return list->size == 0;
}

static void clear_list(struct list *list, bool destroy)
{
        struct node *node = list->head;
        struct node *next = NULL;

        while (node != NULL) {
                next = node->next;

                list_remove(list, node);

                if (destroy) {
                        destroy_node(node);
                }

                node = next;
        }
}

void destroy_list(struct list *list)
{
        clear_list(list, true);

        free(list);
}

void empty_list(struct list *list)
{
        clear_list(list, false);
}
