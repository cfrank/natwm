// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>

struct workspace {
        size_t count;
        size_t active_space_index;
        struct space **spaces;
};

struct space {
        const char *tag_name;
        bool is_visible;
        bool is_focused;
        bool is_floating;
        // TODO Windows
};

struct workspace *workspace_create(size_t count);
struct space *space_create(const char *tag_name);
enum natwm_error workspace_init(const struct natwm_state *state,
                                struct workspace **result);
void workspace_destroy(struct workspace *workspace);
void space_destroy(struct space *space);
