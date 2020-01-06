// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>

#include <common/list.h>

#include "state.h"
#include "workspace.h"

enum server_extension_type {
        RANDR,
        XINERAMA,
        NO_EXTENSION,
};

struct server_extension {
        enum server_extension_type type;
        const xcb_query_extension_reply_t *data_cache;
};

/**
 * This is the representation of a physical screen or "monitor".
 */
struct monitor {
        // `id` is used when reacting to RANDR events. In all other extension
        // contexts this is unused.
        uint32_t id;
        xcb_rectangle_t rect;
        struct workspace *workspace;
};

struct monitor_list {
        struct server_extension *extension;
        struct list *monitors;
};

struct server_extension *server_extension_detect(xcb_connection_t *connection);
const char *server_extension_to_string(enum server_extension_type extension);

struct monitor_list *monitor_list_create(struct server_extension *extension,
                                         struct list *monitors);
struct monitor *
monitor_list_get_active_monitor(const struct monitor_list *monitor_list);
struct monitor *
monitor_list_get_workspace_monitor(const struct monitor_list *monitor_list,
                                   const struct workspace *workspace);
struct monitor *monitor_create(uint32_t id, xcb_rectangle_t rect,
                               struct workspace *workspace);
enum natwm_error monitor_setup(const struct natwm_state *state,
                               struct monitor_list **result);
void monitor_list_destroy(struct monitor_list *monitor_list);
void monitor_destroy(struct monitor *monitor);
