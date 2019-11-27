// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include "stack.h"
#include "tree.h"

struct tree *tree_create(const void *data)
{
        struct tree *tree = malloc(sizeof(struct tree));

        if (tree == NULL) {
                return NULL;
        }

        tree->root = leaf_create(data);

        if (tree->root == NULL) {
                free(tree);

                return NULL;
        }

        tree->size = 1;

        return tree;
}

struct leaf *leaf_create(const void *data)
{
        struct leaf *leaf = malloc(sizeof(struct leaf));

        if (leaf == NULL) {
                return NULL;
        }

        leaf->left = NULL;
        leaf->right = NULL;
        leaf->data = data;

        return leaf;
}

enum natwm_error tree_insert(struct tree *tree, const void *data,
                             struct leaf *append_under,
                             const struct leaf **affected_leaf)
{
        struct leaf *append = tree->root;

        if (append_under != NULL) {
                append = append_under;
        }

        if (append->data == NULL) {
                append->data = data;

                return NO_ERROR;
        }

        append->left = leaf_create(append->data);

        if (append->left == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        append->right = leaf_create(data);

        if (append->right == NULL) {
                free(append->left);

                return MEMORY_ALLOCATION_ERROR;
        }

        append->data = NULL;

        *affected_leaf = append;

        return NO_ERROR;
}

void tree_iterate(struct tree *tree, leaf_callback_t callback)
{
        struct stack *stack = stack_create();
        struct leaf *curr = tree->root;

        while (stack_has_item(stack) || curr != NULL) {
                if (curr != NULL) {
                        stack_push(stack, curr);

                        curr = curr->left;

                        continue;
                }

                struct stack_item *stack_item = stack_pop(stack);

                curr = (struct leaf *)stack_item->data;

                free(stack_item);

                callback(curr);

                curr = curr->right;
        }
}

void tree_destroy(struct tree *tree, leaf_callback_t free_function)
{
        leaf_callback_t callback = leaf_destroy;

        if (free_function != NULL) {
                callback = leaf_destroy;
        }

        tree_iterate(tree, callback);

        free(tree);
}

void leaf_destroy(struct leaf *leaf)
{
        free(leaf);
}
