// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <common/constants.h>

#include "ewmh.h"
#include "monitor.h"
#include "workspace.h"

// Create a simple window for the _NET_SUPPORTING_WM_CHECK property
//
// Return the ID of the created window
static xcb_window_t create_supporting_window(const struct natwm_state *state)
{
        xcb_window_t win = xcb_generate_id(state->xcb);

        xcb_create_window(state->xcb,
                          XCB_COPY_FROM_PARENT,
                          win,
                          state->screen->root,
                          -1,
                          -1,
                          1,
                          1,
                          0,
                          XCB_COPY_FROM_PARENT,
                          state->screen->root_visual,
                          0,
                          NULL);

        return win;
}

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
        pid_t pid = getpid();
        size_t wm_name_len = strlen(NATWM_VERSION_STRING);

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
                state->ewmh->_NET_DESKTOP_VIEWPORT,
                state->ewmh->_NET_CURRENT_DESKTOP,
                state->ewmh->_NET_DESKTOP_NAMES,
                state->ewmh->_NET_ACTIVE_WINDOW,
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

        size_t len = (sizeof(net_atoms) / sizeof(xcb_atom_t));

        xcb_ewmh_set_supported(
                state->ewmh, state->screen_num, (uint32_t)len, net_atoms);

        xcb_ewmh_set_wm_pid(state->ewmh, state->screen->root, (uint32_t)pid);
        xcb_ewmh_set_wm_name(state->ewmh,
                             state->screen->root,
                             (uint32_t)wm_name_len,
                             NATWM_VERSION_STRING);

        xcb_window_t supporting_win = create_supporting_window(state);

        xcb_ewmh_set_supporting_wm_check(
                state->ewmh, state->screen->root, supporting_win);

        // Set the WM name on the supporting win
        xcb_ewmh_set_wm_name(state->ewmh,
                             supporting_win,
                             (uint32_t)wm_name_len,
                             NATWM_VERSION_STRING);

        xcb_ewmh_set_number_of_desktops(state->ewmh,
                                        state->screen_num,
                                        (uint32_t)NATWM_WORKSPACE_COUNT);
}

void ewmh_update_desktop_viewport(const struct natwm_state *state,
                                  const struct monitor_list *list)
{
        size_t num_monitors = list->monitors->size;
        xcb_ewmh_coordinates_t viewports[num_monitors];
        size_t index = 0;

        LIST_FOR_EACH(list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;

                viewports[index].x = (uint32_t)monitor->rect.x;
                viewports[index].y = (uint32_t)monitor->rect.y;

                ++index;
        }

        xcb_ewmh_set_desktop_viewport(state->ewmh,
                                      state->screen_num,
                                      (uint32_t)num_monitors,
                                      viewports);
}

void ewmh_update_desktop_names(const struct natwm_state *state,
                               const struct workspace_list *list)
{
        // Should never happen
        if (list->count < 1) {
                xcb_ewmh_set_desktop_names(
                        state->ewmh, state->screen_num, 0, NULL);

                return;
        }

        char names[(NATWM_WORKSPACE_NAME_MAX_LEN * list->count) + list->count];
        size_t pos = 0;

        for (size_t i = 0; i < list->count; ++i) {
                const char *name = list->workspaces[i]->name;
                size_t name_length = strlen(name);

                memcpy(names + pos, name, name_length);

                names[pos + name_length] = '\0';

                pos += (name_length + 1);
        }

        xcb_ewmh_set_desktop_names(
                state->ewmh, state->screen_num, (uint32_t)(pos - 1), names);
}

void ewmh_update_current_desktop(const struct natwm_state *state,
                                 size_t current_index)
{
        assert(current_index < NATWM_WORKSPACE_COUNT);

        xcb_ewmh_set_current_desktop(
                state->ewmh, state->screen_num, (uint32_t)current_index);
}

void ewmh_destroy(xcb_ewmh_connection_t *ewmh_connection)
{
        xcb_ewmh_connection_wipe(ewmh_connection);

        free(ewmh_connection);
}
