// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

struct node {
        struct node *next;
        struct node *previous;
        void *data;
};

struct list {
        struct node *head;
        struct node *tail;
        size_t size;
};

struct node *create_node(void *data);
void destroy_node(struct node *node);

struct list *create_list(void);
struct node *list_insert_after(struct list *list, struct node *node,
                               void *data);
struct node *list_insert_before(struct list *list, struct node *node,
                                void *data);
struct node *list_insert(struct list *list, void *data);
struct node *list_insert_end(struct list *list, void *data);
void list_remove(struct list *list, struct node *node);
void destroy_list(struct list *list);
void empty_list(struct list *list);
