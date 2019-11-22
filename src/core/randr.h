// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>

#include <common/error.h>

#include "state.h"

enum natwm_error randr_get_screens(const struct natwm_state *state,
                                   xcb_rectangle_t **destination,
                                   size_t *count);

