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
        // Color string (allocated/free'd by config)
        const char *string;
        uint32_t color_value;
};

enum natwm_error color_value_from_string(const char *string,
                                         struct color_value **result);
enum natwm_error color_value_from_config(const struct map *map, const char *key,
                                         struct color_value **result);
bool color_value_has_changed(struct color_value *value,
                             const char *new_string_value);
void color_value_destroy(struct color_value *value);
