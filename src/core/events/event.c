// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>

#include <common/constants.h>
#include <common/logger.h>

#include "event.h"
#include "monitor.h"
#include "randr.h"

static enum natwm_error
handle_randr_notify_event(const struct natwm_state *state,
                          const xcb_randr_notify_event_t *event)
{
        UNUSED_FUNCTION_PARAM(state);
        UNUSED_FUNCTION_PARAM(event);

        LOG_INFO(natwm_logger, "Handle randr notify");

        return NO_ERROR;
}

static enum natwm_error handle_randr_screen_change_event(
        const struct natwm_state *state,
        const xcb_randr_screen_change_notify_event_t *event)
{
        UNUSED_FUNCTION_PARAM(state);
        UNUSED_FUNCTION_PARAM(event);

        LOG_INFO(natwm_logger, "Handle randr screen change");

        return NO_ERROR;
}

static enum natwm_error handle_randr_event(const struct natwm_state *state,
                                           const xcb_generic_event_t *event,
                                           uint8_t event_type)
{
        if (state->monitor_list->extension->type != RANDR) {
                return NO_ERROR;
        }

        const xcb_query_extension_reply_t *cache
                = state->monitor_list->extension->data_cache;

        if (event_type == (cache->first_event + XCB_RANDR_NOTIFY)) {
                return handle_randr_notify_event(
                        state, (const xcb_randr_notify_event_t *)event);
        }

        if (event_type
            == (cache->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY)) {
                return handle_randr_screen_change_event(
                        state,
                        (const xcb_randr_screen_change_notify_event_t *)event);
        }

        return NO_ERROR;
}

enum natwm_error event_handle(const struct natwm_state *state,
                              const xcb_generic_event_t *event)
{
        uint8_t type = (uint8_t)(event->response_type & ~0x80);

        // Handle RANDR possible events
        if (handle_randr_event(state, event, type) != NO_ERROR) {
                return GENERIC_ERROR;
        }

        return NO_ERROR;
}
