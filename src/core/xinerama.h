// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>

#include <common/error.h>

#include "state.h"

bool xinerama_is_active(xcb_connection_t *connection);
enum natwm_error xinerama_get_screens(const struct natwm_state *state,
                                      xcb_rectangle_t **destination,
                                      size_t *length);
