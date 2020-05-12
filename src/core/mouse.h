// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "client.h"
#include "state.h"

#define MOUSE_EVENTS_NUM 2

struct mouse_binding {
        uint8_t pass_event;
        uint16_t mask;
        uint8_t pointer_mode;
        uint8_t keyboard_mode;
        xcb_cursor_t cursor;
        uint8_t button;
        uint16_t modifiers;
};

static const uint16_t DEFAULT_BUTTON_MASK
        = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE;

static const struct mouse_binding mouse_events[MOUSE_EVENTS_NUM] = {
        // A simple single button press with no keyboard modifiers (sets focus)
        {
                .pass_event = 1,
                .mask = DEFAULT_BUTTON_MASK,
                .pointer_mode = XCB_GRAB_MODE_ASYNC,
                .keyboard_mode = XCB_GRAB_MODE_ASYNC,
                .cursor = XCB_NONE,
                .button = XCB_BUTTON_INDEX_1,
                .modifiers = XCB_NONE,
        },
        {
                .pass_event = 0,
                .mask = DEFAULT_BUTTON_MASK,
                .pointer_mode = XCB_GRAB_MODE_ASYNC,
                .keyboard_mode = XCB_GRAB_MODE_ASYNC,
                .cursor = XCB_NONE,
                .button = XCB_BUTTON_INDEX_1,
                .modifiers = XCB_MOD_MASK_1,
        },
};

void mouse_client_listener(const struct natwm_state *state,
                           const struct client *client);
