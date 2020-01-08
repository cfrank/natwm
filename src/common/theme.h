// Copyright 2019 Chris Frank
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
 * A color theme value allows the caller to specify color values for the
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

struct color_theme *color_theme_create(void);
enum natwm_error color_value_from_string(const char *string,
                                         struct color_value **result);
enum natwm_error color_value_from_config(const struct map *map, const char *key,
                                         struct color_value **result);
enum natwm_error color_theme_from_config(const struct map *map, const char *key,
                                         struct color_theme **result);
bool color_value_has_changed(struct color_value *value,
                             const char *new_string_value);
void color_value_destroy(struct color_value *value);
void color_theme_destroy(struct color_theme *theme);
