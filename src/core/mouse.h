// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "client.h"
#include "state.h"

#define MOUSE_EVENTS_NUM 1

#define DEFAULT_BUTTON_MASK                                                    \
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE

struct mouse_binding {
        uint8_t pass_event;
        uint16_t mask;
        uint8_t pointer_mode;
        uint8_t keyboard_mode;
        xcb_cursor_t cursor;
        uint8_t button;
        uint16_t modifiers;
};

static const struct mouse_binding client_focus_event = {
        .pass_event = 1,
        .mask = DEFAULT_BUTTON_MASK,
        .pointer_mode = XCB_GRAB_MODE_SYNC,
        .keyboard_mode = XCB_GRAB_MODE_ASYNC,
        .cursor = XCB_NONE,
        .button = XCB_BUTTON_INDEX_1,
        .modifiers = XCB_NONE,
};

static const struct mouse_binding mouse_events[MOUSE_EVENTS_NUM] = {
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

void mouse_event_grab_button(xcb_connection_t *connection, xcb_window_t window,
                             const struct mouse_binding *binding);

void mouse_initialize_client_listeners(const struct natwm_state *state,
                                       const struct client *client);
