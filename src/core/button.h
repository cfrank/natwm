// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include <common/error.h>

#include "client.h"
#include "state.h"
#include "workspace.h"

#define BUTTON_EVENTS_NUM 2

#define DEFAULT_BUTTON_MASK XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE

struct button_binding {
        uint8_t pass_event;
        uint16_t mask;
        uint8_t pointer_mode;
        uint8_t keyboard_mode;
        xcb_cursor_t cursor;
        uint8_t button;
        uint16_t modifiers;
};

struct toggle_modifiers {
        uint16_t num_lock;
        uint16_t caps_lock;
        uint16_t scroll_lock;
        uint16_t *masks;
};

struct button_state {
        struct toggle_modifiers *modifiers;
        struct client *grabbed_client;
        xcb_window_t resize_helper;
        xcb_rectangle_t *monitor_rect;
        int16_t start_x;
        int16_t start_y;
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

static const struct button_binding button_events[BUTTON_EVENTS_NUM]
        = {{
                   .pass_event = 1,
                   .mask = DEFAULT_BUTTON_MASK | XCB_EVENT_MASK_BUTTON_1_MOTION,
                   .pointer_mode = XCB_GRAB_MODE_ASYNC,
                   .keyboard_mode = XCB_GRAB_MODE_ASYNC,
                   .cursor = XCB_NONE,
                   .button = XCB_BUTTON_INDEX_1,
                   .modifiers = XCB_MOD_MASK_1,
           },
           {
                   .pass_event = 1,
                   .mask = DEFAULT_BUTTON_MASK | XCB_EVENT_MASK_BUTTON_3_MOTION,
                   .pointer_mode = XCB_GRAB_MODE_ASYNC,
                   .keyboard_mode = XCB_GRAB_MODE_ASYNC,
                   .cursor = XCB_NONE,
                   .button = XCB_BUTTON_INDEX_3,
                   .modifiers = XCB_MOD_MASK_1,
           }};

struct button_state *button_state_create(xcb_connection_t *connection);

uint16_t toggle_modifiers_get_clean_mask(const struct toggle_modifiers *modifiers, uint16_t mask);
void button_binding_grab(const struct natwm_state *state, xcb_window_t window,
                         const struct button_binding *binding);
void button_binding_ungrab(const struct natwm_state *state, xcb_window_t window,
                           const struct button_binding *binding);
void button_initialize_client_listeners(const struct natwm_state *state,
                                        const struct client *client);
enum natwm_error button_handle_focus(struct natwm_state *state, struct workspace *workspace,
                                     struct client *client);
enum natwm_error button_handle_grab(struct natwm_state *state,
                                    const xcb_button_press_event_t *event,
                                    xcb_rectangle_t *monitor_rect, struct client *client);
enum natwm_error button_handle_motion(struct natwm_state *state, uint16_t mouse_mask, int16_t x,
                                      int16_t y);
void button_handle_drag_end(struct natwm_state *state);
enum natwm_error button_handle_resize_end(struct natwm_state *state,
                                          const xcb_button_release_event_t *event);

void button_state_destroy(struct natwm_state *state);
