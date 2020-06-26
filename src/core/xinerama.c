// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <xcb/xinerama.h>

#include <common/logger.h>

#include "xinerama.h"

bool xinerama_is_active(xcb_connection_t *connection)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_xinerama_is_active_cookie_t is_active_cookie = xcb_xinerama_is_active(connection);
        xcb_xinerama_is_active_reply_t *is_active_reply
                = xcb_xinerama_is_active_reply(connection, is_active_cookie, &err);

        if (err != XCB_NONE) {
                free(is_active_reply);

                return false;
        }

        bool is_active = (bool)is_active_reply->state;

        free(is_active_reply);

        return is_active;
}

enum natwm_error xinerama_get_screens(const struct natwm_state *state,
                                      xcb_rectangle_t **destination, size_t *count)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_xinerama_query_screens_cookie_t query_screens_cookie
                = xcb_xinerama_query_screens(state->xcb);
        xcb_xinerama_query_screens_reply_t *query_screens_reply
                = xcb_xinerama_query_screens_reply(state->xcb, query_screens_cookie, &err);

        if (err != XCB_NONE) {
                LOG_INFO(natwm_logger, "Failed to get xinerama screens");

                free(query_screens_reply);

                return GENERIC_ERROR;
        }

        xcb_xinerama_screen_info_t *screens_info
                = xcb_xinerama_query_screens_screen_info(query_screens_reply);

        if (screens_info == NULL) {
                LOG_INFO(natwm_logger, "Failed to get xinerama screen info");

                return GENERIC_ERROR;
        }

        int screen_count = xcb_xinerama_query_screens_screen_info_length(query_screens_reply);

        assert(screen_count > 0);

        xcb_rectangle_t *screen_rects = malloc(sizeof(xcb_rectangle_t) * (size_t)screen_count);

        if (screen_rects == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        for (size_t i = 0; i < (size_t)screen_count; ++i) {
                xcb_xinerama_screen_info_t screen_info = screens_info[i];
                xcb_rectangle_t screen_rect = {
                        .x = screen_info.x_org,
                        .y = screen_info.y_org,
                        .width = screen_info.width,
                        .height = screen_info.height,
                };

                screen_rects[i] = screen_rect;
        }

        *destination = screen_rects;
        *count = (size_t)screen_count;

        free(query_screens_reply);

        return NO_ERROR;
}
