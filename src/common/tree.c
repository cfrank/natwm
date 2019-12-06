// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "stack.h"
#include "tree.h"

// When removing a leaf there are two possible options:
//
// 1) If the leaf's sibling contains data then we need to move that data to the
// parent and destroy the leaf
//
// 2) If the leaf's sibling contains children then we need to replace the
// parent with the sibling
static void reposition_leaf(struct leaf *parent, struct leaf *child)
{
        if (child->data != NULL) {
                parent->data = child->data;
                parent->left = NULL;
                parent->right = NULL;

                leaf_destroy(child);
        } else {
                parent->data = NULL;
                parent->left = child->left;
                parent->right = child->right;

                leaf_destroy(child);
        }
}

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
                                  struct leaf **parent)
{
        if (leaf == NULL || leaf->data == NULL || compare_callback == NULL) {
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
                        stack_item_destroy(stack_item);

                        if (prev == NULL) {
                                // No parent was found - maybe searched for root
                                break;
                        }

                        goto found_leaf;
                }

                if (stack_leaf->data == NULL) {
                        prev = stack_leaf;
                }

                curr = stack_leaf->right;

                stack_item_destroy(stack_item);
        }

        stack_destroy(stack);

        return NOT_FOUND_ERROR;

found_leaf:
        stack_destroy(stack);

        *parent = prev;

        return NO_ERROR;
}

enum natwm_error tree_find_sibling(struct tree *tree, struct leaf *leaf,
                                   leaf_compare_callback_t compare_callback,
                                   struct leaf **sibling)
{
        struct leaf *parent = NULL;
        enum natwm_error err
                = tree_find_parent(tree, leaf, compare_callback, &parent);

        if (err != NO_ERROR) {
                return err;
        }

        if (memcmp(parent->left, leaf, sizeof(struct leaf))) {
                *sibling = parent->right;

                return NO_ERROR;
        }

        *sibling = parent->left;

        return NO_ERROR;
}

enum natwm_error tree_remove(struct tree *tree, struct leaf *leaf,
                             leaf_compare_callback_t compare_callback,
                             leaf_data_callback_t free_callback,
                             struct leaf **affected_leaf)
{
        struct leaf *parent = NULL;
        enum natwm_error err
                = tree_find_parent(tree, leaf, compare_callback, &parent);

        if (err == NOT_FOUND_ERROR && leaf->data == NULL) {
                // You cannot remove the root node if it contains
                // children
                return INVALID_INPUT_ERROR;
        } else if (err == NOT_FOUND_ERROR && leaf->data != NULL) {
                // If this is the root node, and it contains data but no
                // children then just remove it's data
                if (free_callback) {
                        free_callback(leaf->data);
                }

                leaf->data = NULL;

                --tree->size;
                *affected_leaf = leaf;

                return NO_ERROR;
        } else if (err != NO_ERROR) {
                return err;
        }

        // Find which side of the parent we are removing
        if (memcmp(parent->left, leaf, sizeof(struct leaf)) == 0) {
                reposition_leaf(parent, parent->right);
        } else {
                reposition_leaf(parent, parent->left);
        }

        if (free_callback) {
                free_callback(leaf->data);
        }

        leaf_destroy(leaf);

        *affected_leaf = parent;

        --tree->size;

        return NO_ERROR;
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
                callback = free_function;
        }

        tree_iterate(tree, tree->root, callback);

        free(tree);
}

void leaf_destroy(struct leaf *leaf)
{
        free(leaf);
}
