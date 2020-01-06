// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#if defined __clang__ || defined __GNUC__
#define ATTR_CONST __attribute__((__const__))
#define ATTR_NONNULL __attribute__((__nonnull__))
#define ATTR_PURE __attribute__((__pure__))
#define ATTR_INLINE inline __attribute__((always_inline))
#else
#define ATTR_CONT
#define ATTR_NONNULL
#define ATTR_PURE
#define ATTR_INLINE inline
#endif

#define UNUSED_FUNCTION_PARAM(param) (void)(param)

#define SET_IF_NON_NULL(dest, value)                                           \
        if ((dest) != NULL) {                                                  \
                (*dest) = value;                                               \
        }

#define NATWM_CONFIG_FILE "natwm/config"

#define NATWM_WORKSPACE_COUNT 10

#define UNFOCUSED_BORDER_WIDTH_CONFIG_STRING "window.unfocused.border_width"
#define FOCUSED_BORDER_WIDTH_CONFIG_STRING "window.focused.border_width"
#define URGENT_BORDER_WIDTH_CONFIG_STRING "window.urgent.border_width"
#define STICKY_BORDER_WIDTH_CONFIG_STRING "window.sticky.border_width"

#define UNFOCUSED_BORDER_COLOR_CONFIG_STRING "window.unfocused.border_color"
#define FOCUSED_BORDER_COLOR_CONFIG_STRING "window.focused.border_color"
#define URGENT_BORDER_COLOR_CONFIG_STRING "window.urgent.border_color"
#define STICKY_BORDER_COLOR_CONFIG_STRING "window.sticky.border_color"

#define UNFOCUSED_BACKGROUND_COLOR_CONFIG_STRING                               \
        "window.unfocused.background_color"
#define FOCUSED_BACKGROUND_COLOR_CONFIG_STRING                                 \
        "window.unfocused.background_color"
#define URGENT_BACKGROUND_COLOR_CONFIG_STRING "window.urgent.background_color"
#define STICKY_BACKGROUND_COLOR_CONFIG_STRING "window.sticky.background_color"
