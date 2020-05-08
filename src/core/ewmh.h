// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <xcb/xcb_ewmh.h>

#include "state.h"

xcb_ewmh_connection_t *ewmh_create(xcb_connection_t *xcb_connection);
void ewmh_init(const struct natwm_state *state);
bool ewmh_is_normal_window(const struct natwm_state *state,
                           xcb_window_t window);
void ewmh_update_active_window(const struct natwm_state *state,
                               xcb_window_t window);
void ewmh_update_desktop_viewport(const struct natwm_state *state,
                                  const struct monitor_list *list);
void ewmh_update_desktop_names(const struct natwm_state *state,
                               const struct workspace_list *list);
void ewmh_update_current_desktop(const struct natwm_state *state,
                                 size_t current_index);
void ewmh_update_window_frame_extents(const struct natwm_state *state,
                                      xcb_window_t window,
                                      uint32_t border_width);
void ewmh_update_window_desktop(const struct natwm_state *state,
                                xcb_window_t window, size_t index);
void ewmh_destroy(xcb_ewmh_connection_t *ewmh_connection);
