// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include "config.h"

struct natwm_state {
        xcb_connection_t *xcb;
        xcb_ewmh_connection_t *ewmh;
        struct map *config;
};

struct natwm_state *natwm_state_create(void);
void natwm_state_destroy(struct natwm_state *state);
