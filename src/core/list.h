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

struct list *create_list(void);
