// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <common/error.h>
#include <core/state.h>

#define GET_EVENT_TYPE(response_type) response_type & ~0x80

enum natwm_error event_handle(struct natwm_state *state,
                              xcb_generic_event_t *event);
