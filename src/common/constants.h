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

#define NATWM_CONFIG_FILE "natwm/config"
