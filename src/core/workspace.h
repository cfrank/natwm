// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>
#include <common/map.h>
#include <common/stack.h>

#include "client.h"
#include "state.h"

struct workspace_list {
        size_t count;
        size_t active_index;
        struct client_theme *theme;
        struct map *client_map;
        struct workspace **workspaces;
};

struct workspace {
        const char *name;
        size_t index;
        bool is_visible;
        bool is_focused;
        struct stack *clients;
        struct client *active_client;
};

struct workspace_list *workspace_list_create(size_t count);
struct workspace *workspace_create(const char *tag_name, size_t index);
enum natwm_error workspace_list_init(const struct natwm_state *state,
                                     struct workspace_list **result);
enum natwm_error workspace_add_client(struct natwm_state *state, size_t index,
                                      struct client *client);
struct workspace *workspace_list_get_focused(const struct workspace_list *list);
struct workspace *
workspace_list_find_client_workspace(const struct workspace_list *list,
                                     const struct client *client);
void workspace_list_destroy(struct workspace_list *workspace_list);
void workspace_destroy(struct workspace *workspace);
