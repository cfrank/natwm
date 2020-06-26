// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>

#include <common/constants.h>
#include <common/logger.h>

#include <core/client.h>
#include <core/ewmh.h>
#include <core/monitor.h>
#include <core/mouse.h>

#include "event.h"
#include "randr-event.h"

static enum natwm_error
event_handle_button_press(struct natwm_state *state,
                          xcb_button_press_event_t *event)
{
        return client_handle_button_press(state, event);
}

static enum natwm_error
event_handle_client_message(struct natwm_state *state,
                            xcb_client_message_event_t *event)
{
        if (event->format != 32) {
                // We currently only support format 32
                return NO_ERROR;
        }

        xcb_window_t window = event->window;

        if (event->type == state->ewmh->_NET_ACTIVE_WINDOW) {
                return client_focus_window(state, window);
        } else if (event->type == state->ewmh->_NET_CLOSE_WINDOW) {
                xcb_destroy_window(state->xcb, window);
        } else if (event->type == state->ewmh->_NET_CURRENT_DESKTOP) {
                uint32_t workspace_index = event->data.data32[0];

                return workspace_list_switch_to_workspace(state,
                                                          workspace_index);
        } else if (event->type == state->ewmh->_NET_WM_DESKTOP) {
                uint32_t workspace_index = event->data.data32[0];

                return client_send_window_to_workspace(
                        state, window, workspace_index);
        } else if (event->type == state->ewmh->_NET_WM_STATE) {
                xcb_atom_t state_atom = (event->data.data32[1] != NO_ERROR)
                        ? event->data.data32[1]
                        : event->data.data32[2];
                xcb_ewmh_wm_state_action_t action = event->data.data32[0];

                if (state_atom == state->ewmh->_NET_WM_STATE_FULLSCREEN) {
                        return client_handle_fullscreen_window(
                                state, action, window);
                }
        }

        return NO_ERROR;
}

static enum natwm_error
event_handle_configure_request(struct natwm_state *state,
                               xcb_configure_request_event_t *event)
{
        return client_configure_window(state, event);
}

static enum natwm_error
event_handle_circulate_request(struct natwm_state *state,
                               xcb_circulate_request_event_t *event)
{
        xcb_circulate_window(state->xcb, event->place, event->window);

        return NO_ERROR;
}

static enum natwm_error
event_handle_destroy_notify(struct natwm_state *state,
                            xcb_destroy_notify_event_t *event)
{
        xcb_window_t window = event->window;

        return client_handle_destroy_notify(state, window);
}

static enum natwm_error event_handle_map_request(struct natwm_state *state,
                                                 xcb_map_request_event_t *event)
{
        xcb_window_t window = event->window;

        if (!ewmh_is_normal_window(state, window)) {
                xcb_map_window(state->xcb, window);

                return NO_ERROR;
        }

        client_register_window(state, window);

        return NO_ERROR;
}

static enum natwm_error event_handle_map_notify(struct natwm_state *state,
                                                xcb_map_notify_event_t *event)
{
        xcb_window_t window = event->window;

        client_handle_map_notify(state, window);

        return NO_ERROR;
}

static enum natwm_error
event_handle_unmap_notify(struct natwm_state *state,
                          xcb_unmap_notify_event_t *event)
{
        xcb_window_t window = event->window;

        client_unmap_window(state, window);

        return NO_ERROR;
}

enum natwm_error event_handle(struct natwm_state *state,
                              xcb_generic_event_t *event)
{
        uint8_t type = (uint8_t)(GET_EVENT_TYPE(event->response_type));

        switch (type) {
        case XCB_BUTTON_PRESS:
                return event_handle_button_press(
                        state, (xcb_button_press_event_t *)event);
                break;
        case XCB_CLIENT_MESSAGE:
                return event_handle_client_message(
                        state, (xcb_client_message_event_t *)event);
                break;
        case XCB_CONFIGURE_REQUEST:
                return event_handle_configure_request(
                        state, (xcb_configure_request_event_t *)event);
                break;
        case XCB_CIRCULATE_REQUEST:
                return event_handle_circulate_request(
                        state, (xcb_circulate_request_event_t *)event);
                break;
        case XCB_DESTROY_NOTIFY:
                return event_handle_destroy_notify(
                        state, (xcb_destroy_notify_event_t *)event);
                break;
        case XCB_MAP_REQUEST:
                return event_handle_map_request(
                        state, (xcb_map_request_event_t *)event);
                break;
        case XCB_MAP_NOTIFY:
                return event_handle_map_notify(state,
                                               (xcb_map_notify_event_t *)event);
                break;
        case XCB_UNMAP_NOTIFY:
                return event_handle_unmap_notify(
                        state, (xcb_unmap_notify_event_t *)event);
                break;
        }

        // If we support randr events we handle those here too
        if (state->monitor_list->extension->type == RANDR) {
                return handle_randr_event(state, event, type);
        }

        return NOT_FOUND_ERROR;
}
