// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

#include "state.h"

enum client_type {
        DOCK,
        TOOLBAR,
        SPLASH,
        NORMAL,
};

struct client {
        size_t space_index;
        xcb_window_t window;
        xcb_rectangle_t rect;
        xcb_rectangle_t floating_rect;
        enum client_type type;
        bool is_sticky;
        bool is_fullscreen;
        bool is_urgent;
};

struct client *client_register_window(const struct natwm_state *state,
                                      xcb_window_t window);
