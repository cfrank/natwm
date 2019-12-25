// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>

#include <common/constants.h>
#include <common/error.h>
#include <common/logger.h>
#include <core/monitor.h>
#include <core/state.h>

#include "randr-event.h"

static enum natwm_error
handle_randr_notify_event(const struct natwm_state *state,
                          const xcb_randr_notify_event_t *event)
{
        UNUSED_FUNCTION_PARAM(state);
        UNUSED_FUNCTION_PARAM(event);

        LOG_INFO(natwm_logger, "Handle notify");

        return NO_ERROR;
}

static enum natwm_error handle_randr_screen_change_event(
        const struct natwm_state *state,
        const xcb_randr_screen_change_notify_event_t *event)
{
        UNUSED_FUNCTION_PARAM(state);
        UNUSED_FUNCTION_PARAM(event);

        LOG_INFO(natwm_logger, "Handle screen change");

        return NO_ERROR;
}

enum natwm_error handle_randr_event(const struct natwm_state *state,
                                    const xcb_generic_event_t *event,
                                    uint8_t event_type)
{
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
