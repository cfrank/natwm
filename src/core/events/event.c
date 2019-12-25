// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>

#include <common/constants.h>
#include <common/logger.h>

#include <core/monitor.h>

#include "event.h"
#include "randr-event.h"

enum natwm_error event_handle(const struct natwm_state *state,
                              const xcb_generic_event_t *event)
{
        uint8_t type = (uint8_t)(event->response_type & ~0x80);

        LOG_INFO(natwm_logger, "Handle events");

        // If we are using the randr extension then we support additional events
        // related to the monitors. If we don't support these events we can just
        // return now
        if (state->monitor_list->extension->type != RANDR) {
                return NO_ERROR;
        }

        enum natwm_error err = handle_randr_event(state, event, type);

        if (err != NO_ERROR) {
                return err;
        }

        return NO_ERROR;
}