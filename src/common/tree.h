// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "error.h"

struct leaf {
        struct leaf *parent;
        struct leaf *left;
        struct leaf *right;
        const void *data;
};

struct tree {
        struct leaf *root;
        size_t size;
};

typedef void (*leaf_callback_t)(struct leaf *leaf);
typedef void (*leaf_data_callback_t)(const void *data);
typedef bool (*leaf_compare_callback_t)(struct leaf *one, struct leaf *two);

struct tree *tree_create(const void *data);
struct leaf *leaf_create(const void *data);

enum natwm_error tree_insert(struct tree *tree, struct leaf *append_under,
                             const void *data);
enum natwm_error tree_remove(struct tree *tree, struct leaf *leaf,
                             leaf_data_callback_t free_callback,
                             struct leaf **affected_leaf);
void tree_iterate(struct tree *tree, struct leaf *start,
                  leaf_callback_t callback);
enum natwm_error leaf_find_parent(struct leaf *leaf, struct leaf **parent);
enum natwm_error leaf_find_sibling(struct leaf *leaf, struct leaf **sibling);

void tree_destroy(struct tree *tree, leaf_callback_t free_function);
void leaf_destroy(struct leaf *leaf);
