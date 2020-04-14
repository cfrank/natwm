// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>

#include <common/constants.h>
#include <common/logger.h>

#include <core/client.h>
#include <core/monitor.h>

#include "event.h"
#include "randr-event.h"

static enum natwm_error
event_handle_map_request(const struct natwm_state *state,
                         xcb_map_request_event_t *event)
{
        xcb_window_t window = event->window;
        struct client *client = client_register_window(state, window);

        if (client == NULL) {
                return RESOLUTION_FAILURE;
        }

        return NO_ERROR;
}

enum natwm_error event_handle(const struct natwm_state *state,
                              xcb_generic_event_t *event)
{
        enum natwm_error err = GENERIC_ERROR;
        uint8_t type = (uint8_t)(event->response_type & ~0x80);

        switch (type) {
        case XCB_MAP_REQUEST:
                err = event_handle_map_request(
                        state, (xcb_map_request_event_t *)event);
                break;
        }

        if (err != NO_ERROR) {
                return err;
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
