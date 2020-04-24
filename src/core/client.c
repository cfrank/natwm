// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <xcb/xcb_icccm.h>

#include <common/constants.h>
#include <common/logger.h>

#include "client.h"
#include "ewmh.h"
#include "monitor.h"
#include "workspace.h"

static enum natwm_error get_window_rect(xcb_connection_t *connection,
                                        xcb_window_t window,
                                        xcb_rectangle_t *result)
{
        xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, window);
        xcb_get_geometry_reply_t *reply
                = xcb_get_geometry_reply(connection, cookie, NULL);

        if (reply == NULL) {
                return RESOLUTION_FAILURE;
        }

        xcb_rectangle_t rect = {
                .width = reply->width,
                .height = reply->height,
                .x = reply->x,
                .y = reply->y,
        };

        *result = rect;

        free(reply);

        return NO_ERROR;
}

static xcb_window_t create_frame_window(const struct natwm_state *state,
                                        xcb_rectangle_t rect,
                                        const struct client_theme *theme)
{
        // Create the parent which will contain the window decorations
        xcb_window_t frame = xcb_generate_id(state->xcb);
        uint32_t mask
                = XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
        uint32_t values[] = {
                theme->color->unfocused->color_value,
                XCB_EVENT_MASK_STRUCTURE_NOTIFY
                        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                        | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                state->screen->default_colormap,
        };

        xcb_create_window(state->xcb,
                          state->screen->root_depth,
                          frame,
                          state->screen->root,
                          rect.x,
                          rect.y,
                          rect.width,
                          rect.height,
                          theme->border_width->unfocused,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          state->screen->root_visual,
                          mask,
                          values);

        xcb_icccm_set_wm_class(
                state->xcb, frame, sizeof(FRAME_CLASS_NAME), FRAME_CLASS_NAME);

        return frame;
}

static enum natwm_error reparent_window(xcb_connection_t *connection,
                                        xcb_window_t parent, xcb_window_t child)
{
        xcb_void_cookie_t cookie
                = xcb_reparent_window_checked(connection, child, parent, 0, 0);
        xcb_generic_error_t *err = xcb_request_check(connection, cookie);

        if (err != XCB_NONE) {
                free(err);

                return RESOLUTION_FAILURE;
        }

        return NO_ERROR;
}

static void update_theme(const struct natwm_state *state,
                         const struct client *client, uint16_t border_width,
                         uint32_t border_color)
{
        uint32_t values[] = {
                border_width,
        };

        xcb_configure_window(state->xcb,
                             client->frame,
                             XCB_CONFIG_WINDOW_BORDER_WIDTH,
                             values);

        if (border_width > 0) {
                xcb_change_window_attributes(state->xcb,
                                             client->frame,
                                             XCB_CW_BORDER_PIXEL,
                                             &border_color);
        }

        client_update_hints(state, client, FRAME_EXTENTS);
}

static void update_stack_mode(const struct natwm_state *state,
                              const struct client *client,
                              xcb_stack_mode_t stack_mode)
{
        uint32_t values[] = {
                stack_mode,
        };

        xcb_configure_window(state->xcb,
                             client->frame,
                             XCB_CONFIG_WINDOW_STACK_MODE,
                             values);

        xcb_flush(state->xcb);
}

struct client *client_create(xcb_window_t window, xcb_rectangle_t rect)
{
        struct client *client = malloc(sizeof(struct client));

        if (client == NULL) {
                return NULL;
        }

        client->window = window;
        client->rect = rect;
        client->is_focused = false;
        client->state = CLIENT_NORMAL;

        return client;
}

struct client *client_register_window(struct natwm_state *state,
                                      xcb_window_t window)
{
        struct workspace *focused_workspace
                = workspace_list_get_focused(state->workspace_list);
        struct monitor *workspace_monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, focused_workspace);

        if (focused_workspace == NULL || workspace_monitor == NULL) {
                // Should not happen
                LOG_WARNING(natwm_logger,
                            "Failed to regiser window - Invalid focused "
                            "workspace or monitor");
                return NULL;
        }

        // First get the rect for the window
        xcb_rectangle_t rect = {0};

        if (get_window_rect(state->xcb, window, &rect) != NO_ERROR) {
                return NULL;
        }

        // Adjust window rect to fit workspace monitor
        xcb_rectangle_t normalized_rect
                = client_clamp_rect_to_monitor(rect, workspace_monitor->rect);

        struct client *client = client_create(window, normalized_rect);

        if (client == NULL) {
                return NULL;
        }

        // Load the client theme from the workspace list
        struct client_theme *theme = state->workspace_list->theme;

        client->frame = create_frame_window(state, client->rect, theme);

        if (reparent_window(state->xcb, client->frame, client->window)
            != NO_ERROR) {
                // Failed to set the frame as the windows parent
                LOG_WARNING(natwm_logger, "Failed to reparent window");

                goto handle_error;
        }

        xcb_change_save_set(state->xcb, XCB_SET_MODE_INSERT, client->window);

        if (workspace_add_client(state, focused_workspace, client)
            != NO_ERROR) {
                LOG_WARNING(natwm_logger, "Failed to add client to workspace");

                goto handle_error;
        }

        client_update_hints(state, client, CLIENT_HINTS_ALL);

        xcb_map_window(state->xcb, client->frame);
        xcb_map_window(state->xcb, client->window);

        xcb_flush(state->xcb);

        return client;

handle_error:
        xcb_destroy_window(state->xcb, client->frame);

        client_destroy(client);

        return NULL;
}

enum natwm_error client_unmap_window(struct natwm_state *state,
                                     xcb_window_t window)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, window);

        if (workspace == NULL) {
                // We do not have this window in our registry - ignore
                return NO_ERROR;
        }

        struct client *client = workspace_find_window_client(workspace, window);

        if (client == NULL) {
                LOG_WARNING(natwm_logger, "Failed to find client during unmap");
                return NOT_FOUND_ERROR;
        }

        client->state = CLIENT_HIDDEN;

        workspace_update_focused(state, workspace);

        xcb_unmap_window(state->xcb, client->frame);

        return NO_ERROR;
}

xcb_rectangle_t client_clamp_rect_to_monitor(xcb_rectangle_t window_rect,
                                             xcb_rectangle_t monitor_rect)
{
        xcb_rectangle_t new_rect = {
                .width = MIN(monitor_rect.width, window_rect.width),
                .height = MIN(monitor_rect.height, window_rect.height),
                .x = MAX(monitor_rect.x, window_rect.x),
                .y = MAX(monitor_rect.y, window_rect.y),
        };

        return new_rect;
}

uint16_t client_get_active_border_width(const struct client_theme *theme,
                                        const struct client *client)
{
        if (client->state == CLIENT_URGENT) {
                return theme->border_width->urgent;
        }

        if (client->state == CLIENT_STICKY) {
                return theme->border_width->sticky;
        }

        if (client->is_focused) {
                return theme->border_width->focused;
        }

        return theme->border_width->unfocused;
}

struct color_value *
client_get_active_border_color(const struct client_theme *theme,
                               const struct client *client)
{
        if (client->state == CLIENT_URGENT) {
                return theme->color->urgent;
        }

        if (client->state == CLIENT_STICKY) {
                return theme->color->sticky;
        }

        if (client->is_focused) {
                return theme->color->focused;
        }

        return theme->color->unfocused;
}

void client_set_focused(const struct natwm_state *state, struct client *client)
{
        if (client == NULL || client->is_focused) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->is_focused = true;

        ewmh_update_active_window(state, client->window);

        update_stack_mode(state, client, XCB_STACK_MODE_ABOVE);

        if (client->state != CLIENT_NORMAL) {
                return;
        }

        update_theme(state,
                     client,
                     theme->border_width->focused,
                     theme->color->focused->color_value);
}

void client_set_unfocused(const struct natwm_state *state,
                          struct client *client)
{
        if (client == NULL || client->is_focused == false) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->is_focused = false;

        update_stack_mode(state, client, XCB_STACK_MODE_BELOW);

        if (client->state != CLIENT_NORMAL) {
                return;
        }

        update_theme(state,
                     client,
                     theme->border_width->unfocused,
                     theme->color->unfocused->color_value);
}

enum natwm_error client_update_hints(const struct natwm_state *state,
                                     const struct client *client,
                                     enum client_hints hints)
{
        if (!client) {
                return INVALID_INPUT_ERROR;
        }

        if (hints & FRAME_EXTENTS) {
                struct client_theme *theme = state->workspace_list->theme;
                uint32_t border_width
                        = client_get_active_border_width(theme, client);

                ewmh_update_window_frame_extents(
                        state, client->window, border_width);
        }

        if (hints & WM_DESKTOP) {
                struct workspace *workspace
                        = workspace_list_find_client_workspace(
                                state->workspace_list, client);

                if (workspace == NULL) {
                        LOG_INFO(natwm_logger,
                                 "Failed to find current desktop");

                        return RESOLUTION_FAILURE;
                }

                ewmh_update_window_desktop(
                        state, client->window, workspace->index);
        }

        return NO_ERROR;
}

enum natwm_error client_theme_create(const struct map *config_map,
                                     struct client_theme **result)
{
        struct client_theme *theme = malloc(sizeof(struct client_theme));

        if (theme == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        enum natwm_error err = GENERIC_ERROR;

        err = border_theme_from_config(config_map,
                                       WINDOW_BORDER_WIDTH_CONFIG_STRING,
                                       &theme->border_width);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_theme_from_config(
                config_map, WINDOW_BORDER_COLOR_CONFIG_STRING, &theme->color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        *result = theme;

        return NO_ERROR;

handle_error:
        client_theme_destroy(theme);

        LOG_ERROR(natwm_logger, "Failed to create client theme");

        return INVALID_INPUT_ERROR;
}

void client_theme_destroy(struct client_theme *theme)
{
        if (theme->border_width != NULL) {
                border_theme_destroy(theme->border_width);
        }

        if (theme->color != NULL) {
                color_theme_destroy(theme->color);
        }

        free(theme);
}

void client_destroy(struct client *client)
{
        free(client);
}
