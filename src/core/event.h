// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <common/error.h>
#include <core/state.h>

enum natwm_error event_handle(const struct natwm_state *state,
                              xcb_generic_event_t *event);
