// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>

#include "state.h"

struct tile {
        // Parent window
        xcb_window_t parent_window;
        // Child (client) window - NULL if empty
        xcb_window_t *client;
        // Dimentions of the tile when floating
        xcb_rectangle_t floating_rect;
        // Dimentions of the tile when tiled. If the tile is empty and
        // is_floating == true this will be 0 and the tile will not be visible
        // to the user
        xcb_rectangle_t tiled_rect;
        bool is_floating;
};

struct tile *tile_create(xcb_window_t *window);
enum natwm_error get_next_tile_rect(const struct natwm_state *state,
                                    xcb_rectangle_t *result);
enum natwm_error tile_init(const struct natwm_state *state, struct tile *tile);
void tile_destroy(struct tile *tile);
