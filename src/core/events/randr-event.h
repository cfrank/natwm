// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/randr.h>

#include <common/error.h>
#include <core/state.h>

enum natwm_error handle_randr_event(const struct natwm_state *state,
                                    const xcb_generic_event_t *event,
                                    uint8_t event_type);
