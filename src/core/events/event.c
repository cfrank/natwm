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

enum natwm_error handle_map_request_event(const struct natwm_state *state,
                                          const xcb_map_request_event_t *event)
{
        struct client *client;
        enum natwm_error err = client_register(state, event->window, &client);

        if (err != NO_ERROR) {
                return err;
        }

        if (client == NULL) {
                // We might be ignoring the window
                return NO_ERROR;
        }

        return NO_ERROR;
}

enum natwm_error event_handle(const struct natwm_state *state,
                              const xcb_generic_event_t *event)
{
        uint8_t type = (uint8_t)(event->response_type & ~0x80);

        switch (type) {
        case XCB_MAP_REQUEST:
                return handle_map_request_event(
                        state, (const xcb_map_request_event_t *)event);
                break;
        default:
                break;
        }

        // If we are using the randr extension then we support additional events
        // related to the monitors. If we don't support these events we can just
        // return now
        if (state->monitor_list->extension->type != RANDR) {
                return NO_ERROR;
        }

        return handle_randr_event(state, event, type);
}
