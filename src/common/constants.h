// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#if defined __clang__ || defined __GNUC__
#define ATTR_NONNULL __attribute__((__nonnull__))
#define ATTR_PURE __attribute__((__pure__))
#define ATTR_CONST __attribute__((__const__))
#else
#define ATTR_NONNULL
#define ATTR_PURE
#endif

#define NATWM_CONFIG_FILE "natwm/config"
