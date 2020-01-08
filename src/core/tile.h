// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>
#include <common/theme.h>

#include "state.h"

enum tile_state {
        TILE_FOCUSED,
        TILE_UNFOCUSED,
        TILE_URGENT,
        TILE_STICKY,
};

struct tile_theme {
        uint16_t border_width;
        struct color_theme *border_color;
        struct color_theme *background_color;
};

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
        enum tile_state state;
};

// Forward declare workspace
struct workspace;

struct tile *tile_create(xcb_window_t *window);
enum natwm_error get_next_tiled_rect(const struct natwm_state *state,
                                     const struct workspace *workspace,
                                     xcb_rectangle_t *result);
struct tile *tile_register_client(const struct natwm_state *state,
                                  xcb_window_t *client);
enum natwm_error attach_tiles_to_workspace(const struct natwm_state *state);
enum natwm_error tile_theme_init(const struct map *config_map,
                                 struct tile_theme **result);
void tile_theme_destroy(struct tile_theme *cache);
void tile_destroy(struct tile *tile);
