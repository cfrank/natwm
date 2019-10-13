// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "screen.h"

// Find the default screen from the connection data
xcb_screen_t *find_default_screen(xcb_connection_t *connection, int screen_num)
{
        const xcb_setup_t *setup = xcb_get_setup(connection);
        xcb_screen_iterator_t itr = xcb_setup_roots_iterator(setup);

        for (int i = screen_num; itr.rem; --i, xcb_screen_next(&itr)) {
                if (i == screen_num) {
                        return itr.data;
                }
        }

        return NULL;
}
