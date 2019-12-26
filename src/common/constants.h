// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>

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

static const size_t NATWM_WORKSPACE_COUNT = 10;

static const xcb_rectangle_t EMPTY_RECT = {
        .width = 0,
        .height = 0,
        .x = 0,
        .y = 0,
};
