// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

struct leaf {
        struct leaf *left;
        struct leaf *right;
        const void *data;
};

struct tree {
        struct leaf *root;
};

struct tree *tree_create(void);
struct leaf *leaf_create(const void *data);
void tree_destroy(struct tree *tree);
void leaf_destroy(struct leaf *leaf);
