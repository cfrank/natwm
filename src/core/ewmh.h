// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb_ewmh.h>

#include "state.h"

xcb_ewmh_connection_t *ewmh_create(xcb_connection_t *xcb_connection);
void ewmh_init(const struct natwm_state *state);
void ewmh_destroy(xcb_ewmh_connection_t *ewmh_connection);
