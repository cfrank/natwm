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

        // A tree with a root node of NULL has a size of 0
        if (data) {
                tree->size = 1;
        } else {
                tree->size = 0;
        }

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

// This function makes a big assumption - that we will never want to specify
// where data will be placed. At this current time I am going to force the user
// to split frame and if they don't like how it is laid out, manually swap the
// frames so they are in the correct place.
//
// This might turn out to be annoying for the user - but it's an easy fix if so
enum natwm_error tree_insert(struct tree *tree, struct leaf *append_under,
                             const void *data)
{
        struct leaf *append = tree->root;

        if (append_under != NULL) {
                append = append_under;
        }

        // In our use case you should not be able to insert a leaf under a leaf
        // which is not either empty or able to accept a new leaf to it's right
        // or left
        // If this is attempted return an error
        if (append->data == NULL
            && (append->left != NULL || append->right != NULL)) {
                return CAPACITY_ERROR;
        }

        // At this point if we are appending into a node with no data then
        // it's an empty node and we can just set the data on the node
        if (append->data == NULL) {
                append->data = data;

                ++tree->size;

                return NO_ERROR;
        }

        // At this point we know that we have data directly on the leaf and
        // both child leafs are empty.
        // Move the data from the leaf to it's left child leaf and insert the
        // new data onto it's right child leaf
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

        ++tree->size;

        return NO_ERROR;
}

enum natwm_error tree_find_parent(struct tree *tree, struct leaf *leaf,
                                  leaf_compare_callback_t compare_callback,
                                  const struct leaf **parent)
{
        if (leaf == NULL) {
                return INVALID_INPUT_ERROR;
        }

        struct stack *stack = stack_create();
        struct leaf *curr = tree->root;
        struct leaf *prev = NULL;

        while (stack_has_item(stack) || curr != NULL) {
                if (curr != NULL) {
                        stack_push(stack, curr);

                        if (curr->data == NULL) {
                                // If we don't have data then we have children,
                                // so mark this leaf as a potential parent
                                prev = curr;
                        }

                        curr = curr->left;

                        continue;
                }

                struct stack_item *stack_item = stack_pop(stack);
                struct leaf *stack_leaf = (struct leaf *)stack_item->data;

                if (compare_callback(leaf, stack_leaf)) {
                        *parent = prev;

                        stack_item_destroy(stack_item);
                        stack_destroy(stack);

                        return NO_ERROR;
                }

                if (stack_leaf->data == NULL) {
                        prev = stack_leaf;
                }

                curr = stack_leaf->right;

                stack_item_destroy(stack_item);
        }

        stack_destroy(stack);

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

                struct leaf *tmp = curr;

                curr = curr->right;

                callback(tmp);

                stack_item_destroy(stack_item);
        }

        stack_destroy(stack);
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
