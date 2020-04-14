// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>
#include <common/stack.h>

#include "client.h"
#include "state.h"

struct workspace_list {
        size_t count;
        size_t active_index;
        struct client_theme *theme;
        struct workspace **workspaces;
};

struct workspace {
        const char *name;
        bool is_visible;
        bool is_focused;
        bool is_floating;
        struct stack *clients;
        struct client *active_client;
};

struct workspace_list *workspace_list_create(size_t count);
struct workspace *workspace_create(const char *tag_name);
enum natwm_error workspace_list_init(const struct natwm_state *state,
                                     struct workspace_list **result);
struct workspace *workspace_list_get_focused(struct workspace_list *list);
void workspace_list_destroy(struct workspace_list *workspace_list);
void workspace_destroy(struct workspace *workspace);
