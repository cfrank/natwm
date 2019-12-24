// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/randr.h>
#include <xcb/xcb.h>

#include <common/error.h>

#include "state.h"

struct randr_monitor {
        xcb_randr_crtc_t id;
        xcb_rectangle_t rect;
};

enum natwm_error randr_get_screens(const struct natwm_state *state,
                                   struct randr_monitor ***result,
                                   size_t *length);
void randr_monitor_destroy(struct randr_monitor *monitor);
