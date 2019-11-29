// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "error.h"

struct leaf {
        struct leaf *left;
        struct leaf *right;
        const void *data;
};

struct tree {
        struct leaf *root;
        size_t size;
};

typedef void (*leaf_callback_t)(struct leaf *leaf);
typedef bool (*leaf_compare_callback_t)(struct leaf *leaf);

struct tree *tree_create(const void *data);
struct leaf *leaf_create(const void *data);

enum natwm_error tree_insert(struct tree *tree, const void *data,
                             struct leaf *append_under,
                             const struct leaf **affected_leaf);
enum natwm_error tree_remove_leaf(struct tree *tree,
                                  leaf_compare_callback_t compare_callback,
                                  leaf_callback_t free_function,
                                  const struct leaf **affected_leaf);

void tree_iterate(struct tree *tree, struct leaf *start,
                  leaf_callback_t callback);

void tree_destroy(struct tree *tree, leaf_callback_t free_function);
void leaf_destroy(struct leaf *leaf);
