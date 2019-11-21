// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <xcb/randr.h>

#include <common/logger.h>

#include "randr.h"

enum natwm_error randr_get_screens(const struct natwm_state *state)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_randr_get_screen_resources_cookie_t resources_cookie
                = xcb_randr_get_screen_resources(state->xcb,
                                                 state->screen->root);
        xcb_randr_get_screen_resources_reply_t *resources_reply
                = xcb_randr_get_screen_resources_reply(
                        state->xcb, resources_cookie, &err);

        if (err != XCB_NONE) {
                free(resources_reply);

                return GENERIC_ERROR;
        }

        int screen_count = xcb_randr_get_screen_resources_outputs_length(
                resources_reply);

        assert(screen_count > 0);

        LOG_INFO(natwm_logger, "Found %d randr screens.", screen_count);

        free(resources_reply);

        return NO_ERROR;
}
