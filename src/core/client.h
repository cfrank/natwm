// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include <common/error.h>
#include <common/map.h>
#include <common/theme.h>

#include "state.h"

// Provided by workspace.h
struct monitor;

enum client_state {
        CLIENT_URGENT = 1U << 0U,
        CLIENT_STICKY = 1U << 1U,
        CLIENT_HIDDEN = 1U << 2U,
        CLIENT_OFF_SCREEN = 1U << 3U,
        CLIENT_NORMAL = 1U << 4U,
};

enum client_hints {
        FRAME_EXTENTS = 1U << 0U,
        WM_ALLOWED_ACTIONS = 1U << 1U,
        WM_DESKTOP = 1U << 2U,
        CLIENT_HINTS_ALL = FRAME_EXTENTS | WM_ALLOWED_ACTIONS | WM_DESKTOP,
};

/**
 * This is the global theme for all clients.
 *
 * The values are user configurable and define the look and feel of the client
 * when it is rendered to the screen.
 */
struct client_theme {
        struct border_theme *border_width;
        struct color_theme *color;
};

struct client {
        xcb_window_t window;
        xcb_rectangle_t rect;
        xcb_size_hints_t *size_hints;
        bool is_focused;
        bool is_fullscreen;
        enum client_state state;
};

struct client *client_create(xcb_window_t window, xcb_rectangle_t rect,
                             xcb_size_hints_t *hints);
struct client *client_register_window(struct natwm_state *state,
                                      xcb_window_t window);
enum natwm_error client_handle_button_press(struct natwm_state *state,
                                            xcb_button_press_event_t *event);
enum natwm_error client_configure_window(struct natwm_state *state,
                                         xcb_configure_request_event_t *event);
void client_configure_window_rect(xcb_connection_t *connection,
                                  xcb_window_t window, xcb_rectangle_t rect,
                                  uint32_t border_width);
void client_map(const struct natwm_state *state, struct client *client,
                const struct monitor *monitor);
enum natwm_error client_handle_map_notify(const struct natwm_state *state,
                                          xcb_window_t window);
enum natwm_error client_unmap_window(struct natwm_state *state,
                                     xcb_window_t window);
enum natwm_error client_handle_destroy_notify(struct natwm_state *state,
                                              xcb_window_t window);
xcb_rectangle_t client_initialize_rect(const struct client *client,
                                       const struct monitor *monitor,
                                       uint16_t border_width);
uint16_t client_get_active_border_width(const struct client_theme *theme,
                                        const struct client *client);
struct color_value *
client_get_active_border_color(const struct client_theme *theme,
                               const struct client *client);
enum natwm_error client_unset_fullscreen(const struct natwm_state *state,
                                         struct client *client);
enum natwm_error client_set_fullscreen(const struct natwm_state *state,
                                       struct client *client);
enum natwm_error
client_handle_fullscreen_window(struct natwm_state *state,
                                xcb_ewmh_wm_state_action_t action,
                                xcb_window_t window);
void client_set_window_input_focus(const struct natwm_state *state,
                                   xcb_window_t window);
void client_set_focused(struct natwm_state *state, struct client *client);
void client_set_unfocused(const struct natwm_state *state,
                          struct client *client);
enum natwm_error client_focus_window(struct natwm_state *state,
                                     xcb_window_t window);
enum natwm_error client_update_hints(const struct natwm_state *state,
                                     const struct client *client,
                                     enum client_hints hints);

enum natwm_error client_theme_create(const struct map *config_map,
                                     struct client_theme **result);

void client_theme_destroy(struct client_theme *theme);
void client_destroy(struct client *client);
