// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

struct color_value {
        // Color string (allocated/free'd by config)
        const char *string;
        xcb_pixmap_t pixmap;
};

struct color_value *color_value_from_string(const char *string);
bool color_value_has_changed(struct color_value *value,
                             const char *new_string_value);
void color_value_destroy(struct color_value *value);
