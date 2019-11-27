// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
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

static void leaf_remove(const struct leaf *leaf, struct leaf *parent,
                        bool is_left_side)
{
        assert(parent->data == NULL);
        assert(leaf->left == NULL);
        assert(leaf->right == NULL);

        if (is_left_side) {
                parent->data = parent->right;
        } else {
                parent->data = parent->left;
        }

        parent->left = NULL;
        parent->right = NULL;
}

enum natwm_error tree_remove_leaf(struct tree *tree,
                                  leaf_compare_callback_t compare_callback,
                                  leaf_callback_t free_function,
                                  const struct leaf **affected_leaf)
{
        struct stack *stack = stack_create();
        struct leaf *curr = tree->root;
        struct leaf *prev = NULL;
        bool is_left_side = false;

        while (stack_has_item(stack) || curr != NULL) {
                if (curr != NULL) {
                        stack_push(stack, curr);

                        prev = curr;
                        curr = curr->left;
                        is_left_side = true;

                        continue;
                }

                struct stack_item *stack_item = stack_pop(stack);

                curr = (struct leaf *)stack_item->data;

                if (compare_callback(curr)) {
                        // Remove this item
                        leaf_remove(curr, prev, is_left_side);

                        free_function(curr);

                        leaf_destroy(curr);

                        *affected_leaf = prev;

                        return NO_ERROR;
                }

                stack_item_destroy(stack_item);

                prev = curr;
                curr = curr->right;
                is_left_side = false;
        }

        return NOT_FOUND_ERROR;
}

void tree_iterate(struct tree *tree, struct leaf *start,
                  leaf_callback_t callback)
{
        struct stack *stack = stack_create();
        struct leaf *curr = tree->root;

        if (start != NULL) {
                curr = start;
        }

        while (stack_has_item(stack) || curr != NULL) {
                if (curr != NULL) {
                        stack_push(stack, curr);

                        curr = curr->left;

                        continue;
                }

                struct stack_item *stack_item = stack_pop(stack);

                curr = (struct leaf *)stack_item->data;

                stack_item_destroy(stack_item);

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

        tree_iterate(tree, tree->root, callback);

        free(tree);
}

void leaf_destroy(struct leaf *leaf)
{
        free(leaf);
}
