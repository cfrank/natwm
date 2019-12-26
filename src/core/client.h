// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>

#include "state.h"
#include "workspace.h"

enum client_type {
        CLIENT_TYPE_DOCK,
        CLIENT_TYPE_TOOLBAR,
        CLIENT_TYPE_SPLASH,
        CLIENT_TYPE_NORMAL,
};

enum client_status {
        CLIENT_STATUS_NORMAL,
        CLIENT_STATUS_URGENT,
        CLIENT_STATUS_FOCUSED,
        CLIENT_STATUS_STICKY,
};

/**
 * A client is the frame which surrounds a mapped window. It can also be a
 * empty frame which hasn't been populated with a window. In the case of a
 * empty frame the rect will be the tiled size and the floating rect will
 * be 0
 *
 * When a window is mapped to the screen two sizes will be saved to the client:
 *
 * 1) It's initial requested size will be saved as it's floating rect.
 * 2) It's tile position will be stored as it's rect.
 */
struct client {
        xcb_rectangle_t floating_rect;
        xcb_rectangle_t rect;
        enum client_type type;
        enum client_status status;
        bool is_floating;
        xcb_window_t window;
        const struct workspace *workspace;
};

struct client *client_create(xcb_window_t window,
                             xcb_rectangle_t floating_rect);
enum natwm_error client_register(const struct natwm_state *state,
                                 xcb_window_t window, struct client **result);
enum natwm_error client_set_workspace(struct client *client,
                                      const struct workspace *workspace);
void client_destroy(struct client *client);
