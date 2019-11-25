// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <common/constants.h>
#include <common/logger.h>

#include "client.h"
#include "event.h"

void handle_map_request(const struct natwm_state *state,
                        xcb_map_request_event_t *event)
{
        LOG_INFO(natwm_logger, "Map request");

        struct client *client = client_register_window(state, event->window);

        xcb_map_window(state->xcb, event->window);

        free(client);
}

enum natwm_error event_handle(const struct natwm_state *state,
                              xcb_generic_event_t *event)
{
        switch (event->response_type & ~0x80) {
        case XCB_MAP_REQUEST:
                handle_map_request(state, (xcb_map_request_event_t *)event);

                break;
        default:
                LOG_INFO(natwm_logger, "Received an event");
        }

        return NO_ERROR;
}
