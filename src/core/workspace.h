// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include "list.h"

struct workspace {
        size_t length;
        struct list *spaces;
};

struct space {
        size_t index;
        xcb_rectangle_t rect;
        bool is_visible;
        bool is_focused;
        struct list *clients;
};

struct workspace *workspace_create(xcb_rectangle_t *rects, size_t count);
struct space *space_create(xcb_rectangle_t rect, size_t index);
void workspace_destroy(struct workspace *workspace);
void space_destroy(struct space *space);
