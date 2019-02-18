// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#if defined __clang__ || defined __GNUC__
#define ATTR_NONNULL __attribute__((__nonnull__))
#else
#define ATTR_NONNULL
#endif
