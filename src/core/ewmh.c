// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "ewmh.h"

xcb_ewmh_connection_t *ewmh_init(xcb_connection_t *xcb_connection)
{
        xcb_ewmh_connection_t *ewmh_connection
                = malloc(sizeof(xcb_ewmh_connection_t));

        if (ewmh_connection == NULL) {
                return NULL;
        }

        xcb_intern_atom_cookie_t *cookie
                = xcb_ewmh_init_atoms(xcb_connection, ewmh_connection);

        if (xcb_ewmh_init_atoms_replies(ewmh_connection, cookie, NULL) == 0) {
                // Failed to init ewmh
                free(ewmh_connection);

                return NULL;
        }

        return ewmh_connection;
}
