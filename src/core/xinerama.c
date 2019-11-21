// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

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

