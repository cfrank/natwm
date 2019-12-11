// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>

#include <common/list.h>

#include "state.h"
#include "workspace.h"

enum server_extension {
        RANDR,
        XINERAMA,
        NO_EXTENSION,
};

/**
 * This is the representation of a physical screen or "monitor".
 */
struct monitor {
        // `id` is used when reacting to RANDR events. In all other extension
        // contexts this is unused.
        uint32_t id;
        enum server_extension extension;
        xcb_rectangle_t rect;
        struct space *space;
};

const char *server_extension_to_string(enum server_extension extension);

struct monitor *monitor_create(uint32_t id, enum server_extension extension,
                               xcb_rectangle_t rect, struct space *space);
enum natwm_error monitor_setup(const struct natwm_state *state,
                               struct list **result, size_t *length);
void monitor_destroy(struct monitor *monitor);
void monitor_list_destroy(struct list *monitor_list);
