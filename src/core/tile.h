// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/color.h>
#include <common/error.h>

#include "state.h"

enum tile_state {
        TILE_FOCUSED,
        TILE_UNFOCUSED,
        TILE_URGENT,
        TILE_STICKY,
};

struct tile_settings_cache {
        // Border width
        uint32_t unfocused_border_width;
        uint32_t focused_border_width;
        uint32_t urgent_border_width;
        uint32_t sticky_border_width;

        // Border color
        struct color_value *unfocused_border_color;
        struct color_value *focused_border_color;
        struct color_value *urgent_border_color;
        struct color_value *sticky_border_color;

        // Background color
        struct color_value *unfocused_background_color;
        struct color_value *focused_background_color;
        struct color_value *urgent_background_color;
        struct color_value *sticky_background_color;
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

struct tile *tile_create(xcb_window_t *window);
enum natwm_error tile_settings_cache_init(const struct map *config_map,
                                          struct tile_settings_cache **result);
enum natwm_error get_next_tile_rect(const struct natwm_state *state,
                                    xcb_rectangle_t *result);
enum natwm_error tile_init(const struct natwm_state *state, struct tile *tile);
void tile_settings_cache_destroy(struct tile_settings_cache *cache);
void tile_destroy(struct tile *tile);
