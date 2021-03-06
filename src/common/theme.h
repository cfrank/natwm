// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

#include <common/error.h>
#include <common/map.h>

struct color_value {
        // String representation (useful for diffing)
        const char *string;
        uint32_t color_value;
};

/**
 * A color theme allows the caller to specify color values for the
 * following states:
 *
 * Unfocused
 * Focused
 * Urgent
 * Sticky
 *
 * These values can then be used to style anything which depends on those
 * states. The obvious examples being borders and backgrounds
 */
struct color_theme {
        struct color_value *unfocused;
        struct color_value *focused;
        struct color_value *urgent;
        struct color_value *sticky;
};

/**
 * A border theme allows the caller to specify border width values for the
 * following states:
 *
 * Unfocused
 * Focused
 * Urgent
 * Sticky
 */
struct border_theme {
        uint16_t unfocused;
        uint16_t focused;
        uint16_t urgent;
        uint16_t sticky;
};

/**
 * This is the global theme for all clients.
 *
 * The values are user configurable and define the look and feel of the client
 * when it is rendered to the screen.
 */
struct theme {
        struct border_theme *border_width;
        struct color_theme *color;
        struct color_value *resize_background_color;
        struct color_value *resize_border_color;
};

struct border_theme *border_theme_create(void);
struct color_theme *color_theme_create(void);
struct theme *theme_create(const struct map *config_map);

bool color_value_has_changed(struct color_value *value, const char *new_string_value);
enum natwm_error color_value_from_string(const char *string, struct color_value **result);
enum natwm_error border_theme_from_config(const struct map *map, const char *key,
                                          struct border_theme **result);
enum natwm_error color_value_from_config(const struct map *map, const char *key,
                                         struct color_value **result);
enum natwm_error color_theme_from_config(const struct map *map, const char *key,
                                         struct color_theme **result);

void border_theme_destroy(struct border_theme *theme);
void color_theme_destroy(struct color_theme *theme);
void color_value_destroy(struct color_value *value);
void theme_destroy(struct theme *theme);
