// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb.h>

#include "state.h"

enum screen_extension {
        RANDR,
        XINERAMA,
        NO_EXTENSION,
};

xcb_screen_t *find_default_screen(xcb_connection_t *connection, int screen_num);
enum natwm_error screen_setup(const struct natwm_state *state);
