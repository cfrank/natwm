// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>
#include <xcb/xinerama.h>

#include <common/error.h>
#include <common/logger.h>

#include "randr.h"
#include "screen.h"
#include "xinerama.h"

static enum screen_extension
detect_screen_extension(xcb_connection_t *connection)
{
        const xcb_query_extension_reply_t *cache = XCB_NONE;

        cache = xcb_get_extension_data(connection, &xcb_randr_id);

        if (cache && cache->present) {
                return RANDR;
        }

        cache = xcb_get_extension_data(connection, &xcb_xinerama_id);

        if ((cache && cache->present) && xinerama_is_active(connection)) {
                return XINERAMA;
        }

        return NO_EXTENSION;
}

// Find the default screen from the connection data
xcb_screen_t *find_default_screen(xcb_connection_t *connection, int screen_num)
{
        const xcb_setup_t *setup = xcb_get_setup(connection);
        xcb_screen_iterator_t itr = xcb_setup_roots_iterator(setup);

        for (int i = screen_num; itr.rem; --i, xcb_screen_next(&itr)) {
                if (i == 0) {
                        return itr.data;
                }
        }

        return NULL;
}

const char *screen_extension_to_string(enum screen_extension extension)
{
        switch (extension) {
        case RANDR:
                return "RANDR";
        case XINERAMA:
                return "Xinerama";
        case NO_EXTENSION:
                return "X";
        default:
                return "";
        }

        return "";
}

static enum natwm_error x_get_screens(const struct natwm_state *state,
                                      xcb_rectangle_t **destination,
                                      size_t *length)
{
        xcb_rectangle_t *rects = malloc(sizeof(xcb_rectangle_t));

        xcb_rectangle_t rect = {
                .x = 0,
                .y = 0,
                .width = state->screen->width_in_pixels,
                .height = state->screen->height_in_pixels,
        };

        rects[0] = rect;

        *destination = rects;
        *length = 1;

        return NO_ERROR;
}

enum natwm_error screen_setup(const struct natwm_state *state)
{
        enum screen_extension supported_extension
                = detect_screen_extension(state->xcb);

        // No matter which extension we are dealing with we will end up
        // with an error code, a list of rects, and a count of rects
        enum natwm_error err = GENERIC_ERROR;
        xcb_rectangle_t *rects = NULL;
        size_t rects_length = 0;

        if (supported_extension == RANDR) {
                err = randr_get_screens(state, &rects, &rects_length);
        } else if (supported_extension == XINERAMA) {
                err = xinerama_get_screens(state, &rects, &rects_length);
        } else {
                err = x_get_screens(state, &rects, &rects_length);
        }

        if (err != NO_ERROR || rects == NULL || rects_length == 0) {
                LOG_ERROR(natwm_logger,
                          "Failed to setup %s screen(s)",
                          screen_extension_to_string(supported_extension));

                return err;
        }

        for (size_t i = 0; i < rects_length; ++i) {
                LOG_INFO(natwm_logger,
                         "Found screen: %zux%zu - %d:%d",
                         rects[i].width,
                         rects[i].height,
                         rects[i].x,
                         rects[i].y);
        }

        free(rects);

        return NO_ERROR;
}
