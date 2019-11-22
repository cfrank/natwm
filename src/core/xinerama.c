// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <xcb/xinerama.h>

#include <common/logger.h>

#include "xinerama.h"

bool xinerama_is_active(xcb_connection_t *connection)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_xinerama_is_active_cookie_t is_active_cookie
                = xcb_xinerama_is_active(connection);
        xcb_xinerama_is_active_reply_t *is_active_reply
                = xcb_xinerama_is_active_reply(
                        connection, is_active_cookie, &err);

        LOG_INFO(natwm_logger, "Testing is xinerama is active");

        if (err != XCB_NONE) {
                free(is_active_reply);

                return false;
        }

        bool is_active = (bool)is_active_reply->state;

        free(is_active_reply);

        return is_active;
}

enum natwm_error xinerama_get_screens(const struct natwm_state *state)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_xinerama_query_screens_cookie_t query_screens_cookie
                = xcb_xinerama_query_screens(state->xcb);
        xcb_xinerama_query_screens_reply_t *query_screens_reply
                = xcb_xinerama_query_screens_reply(
                        state->xcb, query_screens_cookie, &err);

        if (err != XCB_NONE) {
                LOG_INFO(natwm_logger, "Failed to get xinerama screens");

                free(query_screens_reply);

                return GENERIC_ERROR;
        }

        // xcb_xinerama_screen_info_t *screens_info =
        // xcb_xinerama_query_screens_screen_info(query_screens_reply);

        int screens_length = xcb_xinerama_query_screens_screen_info_length(
                query_screens_reply);

        assert(screens_length > 0);

        LOG_INFO(natwm_logger, "Found %d xinerama screen(s)", screens_length);

        free(query_screens_reply);

        return NO_ERROR;
}

