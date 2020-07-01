// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include <common/error.h>

#include "client.h"
#include "state.h"
#include "workspace.h"

#define BUTTON_EVENTS_NUM 1

#define DEFAULT_BUTTON_MASK XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE

// Toggleable keys
// https://cgit.freedesktop.org/xorg/proto/x11proto/tree/keysymdef.h
#define NUM_LOCK_KEYSYM 0xff7f
#define CAPS_LOCK_KEYSYM 0xffe5
#define SCROLL_LOCK_KEYSYM 0xff14

struct button_binding {
        uint8_t pass_event;
        uint16_t mask;
        uint8_t pointer_mode;
        uint8_t keyboard_mode;
        xcb_cursor_t cursor;
        uint8_t button;
        uint16_t modifiers;
};

struct button_modifiers {
        uint16_t num_lock;
        uint16_t caps_lock;
        uint16_t scroll_lock;
};

struct button_state {
        struct button_modifiers *modifiers;
        struct client *grabbed_client;
};

static const struct button_binding client_focus_event = {
        .pass_event = 1,
        .mask = XCB_EVENT_MASK_BUTTON_PRESS,
        .pointer_mode = XCB_GRAB_MODE_SYNC,
        .keyboard_mode = XCB_GRAB_MODE_ASYNC,
        .cursor = XCB_NONE,
        .button = XCB_BUTTON_INDEX_1,
        .modifiers = XCB_NONE,
};

static const struct button_binding button_events[BUTTON_EVENTS_NUM] = {
        {
                .pass_event = 1,
                .mask = DEFAULT_BUTTON_MASK,
                .pointer_mode = XCB_GRAB_MODE_ASYNC,
                .keyboard_mode = XCB_GRAB_MODE_ASYNC,
                .cursor = XCB_NONE,
                .button = XCB_BUTTON_INDEX_1,
                .modifiers = XCB_MOD_MASK_1,
        },
};

struct button_state *button_state_create(xcb_connection_t *connection);
uint16_t button_modifiers_get_clean_mask(const struct button_modifiers *modifiers, uint16_t mask);
void button_event_grab(xcb_connection_t *connection, xcb_window_t window,
                       const struct button_binding *binding);

void button_initialize_client_listeners(const struct natwm_state *state,
                                        const struct client *client);

enum natwm_error button_handle_focus(struct natwm_state *state, struct workspace *workspace,
                                     struct client *client);

void button_state_destroy(struct button_state *state);
