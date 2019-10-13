// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <string.h>
#include <unistd.h>

#include "ewmh.h"

xcb_ewmh_connection_t *ewmh_create(xcb_connection_t *xcb_connection)
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

void ewmh_init(const struct natwm_state *state)
{
        // Base info
        uint32_t pid = (uint32_t)getpid(); // set_wm_pid expects uint32_t

        xcb_ewmh_set_wm_pid(state->ewmh, state->screen->root, pid);
        xcb_ewmh_set_wm_name(state->ewmh,
                             state->screen->root,
                             strlen(NATWM_VERSION_STRING),
                             NATWM_VERSION_STRING);

        // A list of supported atoms
        //
        // More info can be found in the spec
        // https://standards.freedesktop.org/wm-spec/wm-spec-latest.html
        xcb_atom_t net_atoms[] = {
                // Root window properties
                state->ewmh->_NET_SUPPORTED,
                state->ewmh->_NET_CLIENT_LIST,
                state->ewmh->_NET_CLIENT_LIST_STACKING,
                state->ewmh->_NET_NUMBER_OF_DESKTOPS,
                state->ewmh->_NET_DESKTOP_GEOMETRY,
                // We don't support large desktops - this will always be 0,0
                state->ewmh->_NET_DESKTOP_VIEWPORT,
                state->ewmh->_NET_CURRENT_DESKTOP,
                state->ewmh->_NET_DESKTOP_NAMES,
                state->ewmh->_NET_ACTIVE_WINDOW,
                state->ewmh->_NET_WORKAREA,
                state->ewmh->_NET_SUPPORTING_WM_CHECK,
                // Root messages
                state->ewmh->_NET_CLOSE_WINDOW,
                state->ewmh->_NET_MOVERESIZE_WINDOW,
                state->ewmh->_NET_REQUEST_FRAME_EXTENTS,
                // Application window properties
                state->ewmh->_NET_WM_NAME,
                state->ewmh->_NET_WM_VISIBLE_NAME,
                state->ewmh->_NET_WM_DESKTOP,
                state->ewmh->_NET_WM_WINDOW_TYPE,
                state->ewmh->_NET_WM_STATE,
                state->ewmh->_NET_WM_ALLOWED_ACTIONS,
                state->ewmh->_NET_WM_STRUT,
                state->ewmh->_NET_WM_STRUT_PARTIAL,
                state->ewmh->_NET_WM_PID,
                // Window types
                // TODO: Add more
                state->ewmh->_NET_WM_WINDOW_TYPE_DOCK,
                state->ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR,
                state->ewmh->_NET_WM_WINDOW_TYPE_SPLASH,
                state->ewmh->_NET_WM_WINDOW_TYPE_NORMAL,
                // Window States
                state->ewmh->_NET_WM_STATE_STICKY,
                state->ewmh->_NET_WM_STATE_MAXIMIZED_VERT,
                state->ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
                state->ewmh->_NET_WM_STATE_FULLSCREEN,
                state->ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
        };

        uint32_t len = (uint32_t)(sizeof(net_atoms) / sizeof(xcb_atom_t));

        xcb_ewmh_set_supported(state->ewmh, state->screen_num, len, net_atoms);
}

void ewmh_destroy(xcb_ewmh_connection_t *ewmh_connection)
{
        xcb_ewmh_connection_wipe(ewmh_connection);

        free(ewmh_connection);
}
