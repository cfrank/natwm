// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>

#include <common/error.h>
#include <common/map.h>
#include <common/theme.h>

#include "state.h"

enum client_state {
        CLIENT_FOCUSED,
        CLIENT_UNFOCUSED,
        CLIENT_URGENT,
        CLIENT_STICKY,
};

/**
 * This is the global theme for all clients.
 *
 * The values are user configurable and define the look and feel of the client
 * when it is rendered to the screen.
 */
struct client_theme {
        struct border_theme *border_width;
        struct color_theme *color;
};

struct client {
        // The parent which contains the window decorations
        xcb_window_t parent;
        xcb_window_t child;
        xcb_rectangle_t rect;
        enum client_state state;
};

struct client *client_create(xcb_window_t window, xcb_rectangle_t rect);
struct client *client_register_window(struct natwm_state *state,
                                      xcb_window_t window);
xcb_rectangle_t client_clamp_rect_to_monitor(xcb_rectangle_t client_rect,
                                             xcb_rectangle_t monitor_rect);
void client_set_focused(const struct natwm_state *state, struct client *client);
void client_set_unfocused(const struct natwm_state *state,
                          struct client *client);

enum natwm_error client_theme_create(const struct map *config_map,
                                     struct client_theme **result);

void client_theme_destroy(struct client_theme *theme);
void client_destroy(struct client *client);
