// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>

#include <common/constants.h>
#include <common/logger.h>

#include <core/client.h>
#include <core/ewmh.h>
#include <core/monitor.h>

#include "event.h"
#include "randr-event.h"

static enum natwm_error
event_handle_configure_request(struct natwm_state *state,
                               xcb_configure_request_event_t *event)
{
        return client_configure_window(state, event);
}

static enum natwm_error
event_handle_destroy_notify(struct natwm_state *state,
                            xcb_destroy_notify_event_t *event)
{
        xcb_window_t window = event->window;
        enum natwm_error err = client_destroy_window(state, window);

        if (err != NO_ERROR) {
                // A failure here would corrupt the integrity of the workspace
                // list.
                return err;
        }

        return NO_ERROR;
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
        enum natwm_error err = GENERIC_ERROR;
        uint8_t type = (uint8_t)(event->response_type & ~0x80);

        switch (type) {
        case XCB_CONFIGURE_REQUEST:
                err = event_handle_configure_request(
                        state, (xcb_configure_request_event_t *)event);
                break;
        case XCB_DESTROY_NOTIFY:
                err = event_handle_destroy_notify(
                        state, (xcb_destroy_notify_event_t *)event);
                break;
        case XCB_MAP_REQUEST:
                err = event_handle_map_request(
                        state, (xcb_map_request_event_t *)event);
                break;
        case XCB_UNMAP_NOTIFY:
                err = event_handle_unmap_notify(
                        state, (xcb_unmap_notify_event_t *)event);
                break;
        }

        // If we are using the randr extension then we support additional events
        // related to the monitors. If we don't support these events we can just
        // return now
        if (state->monitor_list->extension->type != RANDR) {
                return NO_ERROR;
        }

        err = handle_randr_event(state, event, type);

        if (err != NO_ERROR) {
                return err;
        }

        return NO_ERROR;
}
