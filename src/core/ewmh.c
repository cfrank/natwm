// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb_icccm.h>

#include <common/constants.h>

#include "ewmh.h"

static xcb_window_t ewmh_supporting_window = XCB_NONE;

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
        xcb_ewmh_connection_t *ewmh_connection = malloc(sizeof(xcb_ewmh_connection_t));

        if (ewmh_connection == NULL) {
                return NULL;
        }

        xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(xcb_connection, ewmh_connection);

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
                state->ewmh->_NET_WM_DESKTOP,
                state->ewmh->_NET_WM_WINDOW_TYPE,
                state->ewmh->_NET_WM_STATE,
                state->ewmh->_NET_WM_PID,
                // Window types
                // TODO: Add more
                state->ewmh->_NET_WM_WINDOW_TYPE_NORMAL,
                // Window States
                state->ewmh->_NET_WM_STATE_FULLSCREEN,
        };

        size_t len = (sizeof(net_atoms) / sizeof(xcb_atom_t));

        xcb_ewmh_set_supported(state->ewmh, state->screen_num, (uint32_t)len, net_atoms);

        xcb_ewmh_set_wm_pid(state->ewmh, state->screen->root, (uint32_t)pid);
        xcb_ewmh_set_wm_name(
                state->ewmh, state->screen->root, (uint32_t)wm_name_len, NATWM_VERSION_STRING);

        ewmh_supporting_window = create_supporting_window(state);

        xcb_ewmh_set_supporting_wm_check(state->ewmh, state->screen->root, ewmh_supporting_window);
        xcb_ewmh_set_supporting_wm_check(
                state->ewmh, ewmh_supporting_window, ewmh_supporting_window);

        // Set the WM name on the supporting win
        xcb_ewmh_set_wm_name(
                state->ewmh, ewmh_supporting_window, (uint32_t)wm_name_len, NATWM_VERSION_STRING);

        xcb_icccm_set_wm_class(state->xcb,
                               ewmh_supporting_window,
                               sizeof(SUPPORTING_WINDOW_CLASS_NAME),
                               SUPPORTING_WINDOW_CLASS_NAME);

        xcb_ewmh_set_number_of_desktops(
                state->ewmh, state->screen_num, (uint32_t)NATWM_WORKSPACE_COUNT);
}

bool ewmh_is_normal_window(const struct natwm_state *state, xcb_window_t window)
{
        xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_window_type(state->ewmh, window);
        xcb_ewmh_get_atoms_reply_t result;
        uint8_t reply = xcb_ewmh_get_wm_window_type_reply(state->ewmh, cookie, &result, NULL);

        if (reply != 1) {
                // Treat as a normal window
                return true;
        }

        for (size_t i = 0; i < result.atoms_len; ++i) {
                xcb_atom_t type = result.atoms[i];

                // For now we only support normal windows for registering
                if (type != state->ewmh->_NET_WM_WINDOW_TYPE_NORMAL) {
                        xcb_ewmh_get_atoms_reply_wipe(&result);

                        return false;
                }
        }

        xcb_ewmh_get_atoms_reply_wipe(&result);

        return true;
}

void ewmh_add_window_state(const struct natwm_state *state, xcb_window_t window, xcb_atom_t atom)
{
        xcb_change_property(state->xcb,
                            XCB_PROP_MODE_REPLACE,
                            window,
                            state->ewmh->_NET_WM_STATE,
                            XCB_ATOM_ATOM,
                            32,
                            1,
                            &atom);
}

void ewmh_remove_window_state(const struct natwm_state *state, xcb_window_t window)
{
        xcb_change_property(state->xcb,
                            XCB_PROP_MODE_REPLACE,
                            window,
                            state->ewmh->_NET_WM_STATE,
                            XCB_ATOM_ATOM,
                            32,
                            0,
                            NULL);
}

void ewmh_update_active_window(const struct natwm_state *state, xcb_window_t window)
{
        xcb_ewmh_set_active_window(state->ewmh, state->screen_num, window);
}

void ewmh_update_desktop_viewport(const struct natwm_state *state, const struct monitor_list *list)
{
        size_t num_monitors = list->monitors->size;
        xcb_ewmh_coordinates_t viewports[num_monitors];
        size_t index = 0;

        LIST_FOR_EACH(list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;
                xcb_rectangle_t rect = monitor_get_offset_rect(monitor);

                viewports[index].x = (uint32_t)rect.x;
                viewports[index].y = (uint32_t)rect.y;

                ++index;
        }

        xcb_ewmh_set_desktop_viewport(
                state->ewmh, state->screen_num, (uint32_t)num_monitors, viewports);
}

void ewmh_update_desktop_names(const struct natwm_state *state, const struct workspace_list *list)
{
        // Should never happen
        if (list->count < 1) {
                xcb_ewmh_set_desktop_names(state->ewmh, state->screen_num, 0, NULL);

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

        xcb_ewmh_set_desktop_names(state->ewmh, state->screen_num, (uint32_t)(pos - 1), names);
}

void ewmh_update_current_desktop(const struct natwm_state *state, size_t current_index)
{
        assert(current_index < NATWM_WORKSPACE_COUNT);

        xcb_ewmh_set_current_desktop(state->ewmh, state->screen_num, (uint32_t)current_index);
}

void ewmh_update_window_frame_extents(const struct natwm_state *state, xcb_window_t window,
                                      uint32_t border_width)
{
        xcb_ewmh_set_frame_extents(
                state->ewmh, window, border_width, border_width, border_width, border_width);
}

void ewmh_update_window_desktop(const struct natwm_state *state, xcb_window_t window, size_t index)
{
        xcb_ewmh_set_wm_desktop(state->ewmh, window, (uint32_t)index);
}

void ewmh_destroy(struct natwm_state *state)
{
        xcb_ewmh_connection_wipe(state->ewmh);

        if (state->xcb != NULL && ewmh_supporting_window != XCB_NONE) {
                xcb_destroy_window(state->xcb, ewmh_supporting_window);
        }

        free(state->ewmh);
}
