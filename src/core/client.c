// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>

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

static enum natwm_error get_size_hints(xcb_connection_t *connection,
                                       xcb_window_t window,
                                       xcb_size_hints_t *result)
{
        xcb_size_hints_t hints = {0};
        xcb_get_property_cookie_t cookie
                = xcb_icccm_get_wm_normal_hints_unchecked(connection, window);
        uint8_t reply = xcb_icccm_get_wm_normal_hints_reply(
                connection, cookie, &hints, NULL);

        if (reply != 1) {
                return RESOLUTION_FAILURE;
        }

        *result = hints;

        return NO_ERROR;
}

static xcb_get_window_attributes_reply_t *
get_window_attributes(xcb_connection_t *connection, xcb_window_t window)
{
        xcb_get_window_attributes_cookie_t cookie
                = xcb_get_window_attributes(connection, window);
        xcb_get_window_attributes_reply_t *reply
                = xcb_get_window_attributes_reply(connection, cookie, NULL);

        return reply;
}

static xcb_rectangle_t clamp_rect_to_monitor(xcb_rectangle_t rect,
                                             xcb_rectangle_t monitor_rect)
{
        int32_t x = rect.x;
        int32_t y = rect.y;
        int32_t width = rect.width;
        int32_t height = rect.height;
        int32_t end_x_pos = width + x;
        int32_t end_y_pos = height + y;

        if (end_x_pos > monitor_rect.width) {
                int32_t overflow = end_x_pos - monitor_rect.width;
                int32_t new_x = x - overflow;

                x = MAX(monitor_rect.x, new_x);
                width = MIN(monitor_rect.width, width);
        } else {
                x = MAX(monitor_rect.x, x);
        }

        if (end_y_pos > monitor_rect.height) {
                int32_t overflow = end_y_pos - monitor_rect.width;
                int32_t new_y = y - overflow;

                y = MAX(monitor_rect.y, new_y);
                height = MIN(monitor_rect.height, height);
        } else {
                y = MAX(monitor_rect.y, y);
        }

        xcb_rectangle_t new_rect = {
                .x = (int16_t)x,
                .y = (int16_t)y,
                .width = (uint16_t)width,
                .height = (uint16_t)height,
        };

        return new_rect;
}

static void update_theme(const struct natwm_state *state, struct client *client,
                         uint16_t previous_border_width)
{
        uint16_t current_border_width = client_get_active_border_width(
                state->workspace_list->theme, client);
        const struct color_value *border_color = client_get_active_border_color(
                state->workspace_list->theme, client);

        if (previous_border_width == current_border_width) {
                xcb_change_window_attributes(state->xcb,
                                             client->window,
                                             XCB_CW_BORDER_PIXEL,
                                             &border_color->color_value);

                return;
        }

        int32_t border_diff
                = (previous_border_width - current_border_width) * 2;
        int32_t new_width = client->rect.width + border_diff;
        int32_t new_height = client->rect.height + border_diff;

        client->rect.width = (uint16_t)MAX(0, new_width);
        client->rect.height = (uint16_t)MAX(0, new_height);

        // Update the client
        uint16_t client_mask = XCB_CONFIG_WINDOW_WIDTH
                | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
        uint32_t client_values[] = {
                client->rect.width,
                client->rect.height,
                current_border_width,
        };

        xcb_configure_window(
                state->xcb, client->window, client_mask, client_values);

        xcb_change_window_attributes(state->xcb,
                                     client->window,
                                     XCB_CW_BORDER_PIXEL,
                                     &border_color->color_value);

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
                             client->window,
                             XCB_CONFIG_WINDOW_STACK_MODE,
                             values);

        xcb_flush(state->xcb);
}

struct client *client_create(xcb_window_t window, xcb_rectangle_t rect,
                             xcb_size_hints_t hints)
{
        struct client *client = malloc(sizeof(struct client));

        if (client == NULL) {
                return NULL;
        }

        client->window = window;
        client->rect = rect;
        client->size_hints = hints;
        client->is_focused = false;
        client->state = CLIENT_NORMAL;

        return client;
}

enum natwm_error client_destroy_window(struct natwm_state *state,
                                       xcb_window_t window)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, window);

        if (workspace == NULL) {
                // This window is not registered with us
                xcb_destroy_window(state->xcb, window);

                return NO_ERROR;
        }

        struct client *client = workspace_find_window_client(workspace, window);

        if (client == NULL) {
                LOG_WARNING(natwm_logger,
                            "Failed to find client during destroy");

                xcb_destroy_window(state->xcb, window);

                return NOT_FOUND_ERROR;
        }

        enum natwm_error err
                = workspace_remove_client(state, workspace, client);

        if (err != NO_ERROR) {
                LOG_WARNING(natwm_logger,
                            "Failed to remove client from workspace");

                return err;
        }

        xcb_change_save_set(state->xcb, XCB_SET_MODE_DELETE, client->window);

        xcb_destroy_window(state->xcb, client->window);

        client_destroy(client);

        return NO_ERROR;
}

struct client *client_register_window(struct natwm_state *state,
                                      xcb_window_t window)
{
        // Get window attributes
        xcb_get_window_attributes_reply_t *attributes
                = get_window_attributes(state->xcb, window);

        if (attributes == NULL) {
                goto handle_no_register;
        }

        if (attributes->override_redirect) {
                goto handle_no_register;
        }

        free(attributes);

        struct workspace *focused_workspace
                = workspace_list_get_focused(state->workspace_list);
        struct monitor *workspace_monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, focused_workspace);
        enum natwm_error err = GENERIC_ERROR;

        if (focused_workspace == NULL || workspace_monitor == NULL) {
                // Should not happen
                LOG_WARNING(natwm_logger,
                            "Failed to register window - Invalid focused "
                            "workspace or monitor");

                return NULL;
        }

        // First get the rect for the window
        xcb_rectangle_t rect = {0};
        xcb_size_hints_t hints = {0};

        if (get_window_rect(state->xcb, window, &rect) != NO_ERROR) {
                return NULL;
        }

        if (get_size_hints(state->xcb, window, &hints) != NO_ERROR) {
                return NULL;
        }

        struct client *client = client_create(window, rect, hints);

        if (client == NULL) {
                return NULL;
        }

        // Load the client theme from the workspace list
        struct client_theme *theme = state->workspace_list->theme;

        // Adjust window rect to fit workspace monitor
        client->rect = client_initialize_rect(client,
                                              theme->border_width->unfocused,
                                              workspace_monitor->rect);

        // Set the adjusted rect to the client window
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
                | XCB_CONFIG_WINDOW_BORDER_WIDTH;
        uint32_t values[] = {
                (uint16_t)client->rect.x,
                (uint16_t)client->rect.y,
                client->rect.width,
                client->rect.height,
                theme->border_width->unfocused,
        };

        xcb_configure_window(state->xcb, client->window, mask, values);

        xcb_change_save_set(state->xcb, XCB_SET_MODE_INSERT, client->window);

        err = workspace_add_client(state, focused_workspace, client);

        if (err != NO_ERROR) {
                LOG_WARNING(natwm_logger, "Failed to add client to workspace");

                goto handle_error;
        }

        client_update_hints(state, client, CLIENT_HINTS_ALL);

        xcb_map_window(state->xcb, client->window);

        xcb_flush(state->xcb);

        return client;

handle_no_register:
        free(attributes);

        // Handle a case where we should just directly map the window
        xcb_map_window(state->xcb, window);

        return NULL;

handle_error:
        client_destroy(client);

        return NULL;
}

enum natwm_error client_unmap_window(struct natwm_state *state,
                                     xcb_window_t window)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, window);

        if (workspace == NULL) {
                // We do not have this window in our registry

                xcb_unmap_window(state->xcb, window);

                return NO_ERROR;
        }

        struct client *client = workspace_find_window_client(workspace, window);

        if (client == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to find registered client during unmap");

                xcb_unmap_window(state->xcb, window);

                return NOT_FOUND_ERROR;
        }

        client->state = CLIENT_HIDDEN;

        workspace_reset_focus(state, workspace);

        return NO_ERROR;
}

xcb_rectangle_t client_initialize_rect(struct client *client,
                                       uint16_t border_width,
                                       xcb_rectangle_t monitor_rect)
{
        xcb_rectangle_t new_rect = client->rect;

        if (client->size_hints.flags & XCB_ICCCM_SIZE_HINT_US_SIZE) {
                assert(client->size_hints.x <= INT16_MAX);
                assert(client->size_hints.y <= INT16_MAX);

                new_rect.x = (int16_t)client->size_hints.x;
                new_rect.y = (int16_t)client->size_hints.y;
        }

        if (client->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_SIZE) {
                assert(client->size_hints.width <= UINT16_MAX);
                assert(client->size_hints.height <= UINT16_MAX);

                new_rect.width = (uint16_t)client->size_hints.width;
                new_rect.height = (uint16_t)client->size_hints.height;
        }

        new_rect = clamp_rect_to_monitor(new_rect, monitor_rect);

        // Account for initial border_width
        int32_t border_padding = border_width * 2;
        int32_t final_width = new_rect.width - border_padding;
        int32_t final_height = new_rect.height - border_padding;

        new_rect.width = (uint16_t)(MAX(0, final_width));
        new_rect.height = (uint16_t)(MAX(0, final_height));

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

        update_theme(state, client, theme->border_width->unfocused);
}

void client_set_unfocused(const struct natwm_state *state,
                          struct client *client)
{
        if (client == NULL || client->is_focused == false) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->is_focused = false;

        if (client->state != CLIENT_NORMAL) {
                return;
        }

        update_theme(state, client, theme->border_width->focused);
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
