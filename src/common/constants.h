// Copyright 2020 Chris Frank
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

#define SET_IF_NON_NULL(dest, value)                                                               \
        if ((dest) != NULL) {                                                                      \
                (*dest) = value;                                                                   \
        }

// Copied from https://github.com/openbsd/src/blob/master/sys/sys/param.h
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define NATWM_CONFIG_FILE "natwm/natwm.config"

#define SUPPORTING_WINDOW_CLASS_NAME "supporting_window\0" NATWM_VERSION_STRING
#define RESIZE_HELPER_WINDOW_CLASS_NAME "resize_helper_window\0" NATWM_VERSION_STRING

#define NATWM_WORKSPACE_COUNT 10

#define NATWM_WORKSPACE_NAME_MAX_LEN 50

#define DEFAULT_BORDER_WIDTH 1

#define WINDOW_BORDER_COLOR_CONFIG_STRING "window.border_color"
#define WINDOW_BORDER_WIDTH_CONFIG_STRING "window.border_width"
#define RESIZE_BACKGROUND_COLOR_CONFIG_STRING "resize.background_color"
#define RESIZE_BORDER_COLOR_CONFIG_STRING "resize.border_color"
