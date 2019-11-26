// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "tree.h"

struct tree *tree_create(void)
{
        struct tree *tree = malloc(sizeof(struct tree));

        if (tree == NULL) {
                return NULL;
        }

        tree->root = leaf_create(NULL);

        return tree;
}

struct leaf *leaf_create(const void *data)
{
        struct leaf *leaf = malloc(sizeof(struct leaf));

        if (leaf == NULL) {
                return NULL;
        }

        tree->left = NULL;
        tree->right = NULL;
        tree->data = data;

        return tree;
}

void tree_destroy(struct tree *tree)
{
}
