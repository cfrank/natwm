// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <common/constants.h>
#include <common/logger.h>

#include "event.h"

enum natwm_error event_handle(const struct natwm_state *state,
                              xcb_generic_event_t *event)
{
        UNUSED_FUNCTION_PARAM(state);
        UNUSED_FUNCTION_PARAM(event);

        LOG_INFO(natwm_logger, "Received event");

        return NO_ERROR;
}
