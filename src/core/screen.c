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

enum natwm_error screen_setup(const struct natwm_state *state)
{
        enum screen_extension supported_extension
                = detect_screen_extension(state->xcb);

        if (supported_extension == RANDR) {
                randr_get_screens(state);
        } else if (supported_extension == XINERAMA) {
                LOG_INFO(natwm_logger, "Implement xinerama support");
        } else {
                LOG_INFO(natwm_logger, "Implement general support");
        }

        return NO_ERROR;
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
