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
#include "mouse.h"
#include "workspace.h"

static void handle_configure_request(xcb_connection_t *connection,
                                     xcb_configure_request_event_t *event)
{
        uint16_t mask = 0;
        uint32_t values[6];
        size_t value_index = 0;

        // Configure the new window dimentions
        if (event->value_mask & XCB_CONFIG_WINDOW_X) {
                mask |= XCB_CONFIG_WINDOW_X;
                values[value_index] = (uint16_t)event->x;

                value_index++;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_Y) {
                mask |= XCB_CONFIG_WINDOW_Y;
                values[value_index] = (uint16_t)event->y;

                value_index++;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_WIDTH;
                values[value_index] = event->width;

                value_index++;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
                mask |= XCB_CONFIG_WINDOW_HEIGHT;
                values[value_index] = event->height;

                value_index++;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
                mask |= XCB_CONFIG_WINDOW_SIBLING;
                values[value_index] = event->sibling;

                value_index++;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
                mask |= XCB_CONFIG_WINDOW_STACK_MODE;
                values[value_index] = event->stack_mode;

                value_index++;
        }

        if (mask == 0) {
                // Nothing to do (we ignore XCB_CONFIG_WINDOW_BORDER_WIDTH)
                return;
        }

        xcb_configure_window(
                connection, event->window, mask, (uint32_t *)&values);
}

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
                                       xcb_size_hints_t **result)
{
        xcb_size_hints_t *hints = malloc(sizeof(xcb_size_hints_t));

        if (hints == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        xcb_get_property_cookie_t cookie
                = xcb_icccm_get_wm_normal_hints_unchecked(connection, window);
        uint8_t reply = xcb_icccm_get_wm_normal_hints_reply(
                connection, cookie, hints, NULL);

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

static void update_theme(const struct natwm_state *state, struct client *client,
                         uint16_t previous_border_width)
{
        if (client->is_fullscreen) {
                // No need to upate theme if client is fullscreen
                return;
        }

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
                              xcb_window_t window, xcb_stack_mode_t stack_mode)
{
        uint32_t values[] = {
                stack_mode,
        };

        xcb_configure_window(
                state->xcb, window, XCB_CONFIG_WINDOW_STACK_MODE, values);

        xcb_flush(state->xcb);
}

struct client *client_create(xcb_window_t window, xcb_rectangle_t rect,
                             xcb_size_hints_t *hints)
{
        struct client *client = malloc(sizeof(struct client));

        if (client == NULL) {
                return NULL;
        }

        client->window = window;
        client->rect = rect;
        client->size_hints = hints;

        // Initialize size_hints
        if (!(client->size_hints->flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)) {
                client->size_hints->max_width = UINT16_MAX;
                client->size_hints->max_height = UINT16_MAX;
        }

        if (!(client->size_hints->flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)) {
                client->size_hints->min_width = 0;
                client->size_hints->min_height = 0;
        }

        client->is_focused = false;
        client->is_fullscreen = false;
        client->state = CLIENT_NORMAL;

        return client;
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
        xcb_size_hints_t *hints = NULL;

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
        xcb_rectangle_t workspace_monitor_rect
                = monitor_get_offset_rect(workspace_monitor);

        // Adjust window rect to fit workspace monitor
        client->rect = client_initialize_rect(
                client, theme->border_width->unfocused, workspace_monitor_rect);

        // Listen for button events
        mouse_initialize_client_listeners(state, client);

        xcb_change_save_set(state->xcb, XCB_SET_MODE_INSERT, client->window);

        client_map(state, client, workspace_monitor->rect);

        err = workspace_add_client(state, focused_workspace, client);

        if (err != NO_ERROR) {
                LOG_WARNING(natwm_logger, "Failed to add client to workspace");

                xcb_unmap_window(state->xcb, client->window);

                goto handle_error;
        }

        client_update_hints(state, client, CLIENT_HINTS_ALL);

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

enum natwm_error client_handle_button_press(struct natwm_state *state,
                                            xcb_button_press_event_t *event)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, event->event);

        if (workspace == NULL) {
                // Not registered with us - just pass it along
                return NO_ERROR;
        }

        struct client *client
                = workspace_find_window_client(workspace, event->event);

        if (client == NULL) {
                return RESOLUTION_FAILURE;
        }

        if (event->state == XCB_NONE) {
                return workspace_focus_client(state, workspace, client);
        }

        return NO_ERROR;
}

enum natwm_error client_configure_window(struct natwm_state *state,
                                         xcb_configure_request_event_t *event)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, event->window);

        if (workspace == NULL) {
                // window is not registered with us - just pass it along
                goto handle_not_registered;
        }

        struct client *client
                = workspace_find_window_client(workspace, event->window);
        struct monitor *monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, workspace);

        if (client == NULL || monitor == NULL) {
                return RESOLUTION_FAILURE;
        }

        xcb_rectangle_t new_rect = client->rect;
        xcb_configure_request_event_t new_event = *event;

        if (event->value_mask & XCB_CONFIG_WINDOW_X) {
                new_rect.x = event->x;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_Y) {
                new_rect.y = event->y;
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
                if (event->width < client->size_hints->min_width) {
                        new_rect.width
                                = (uint16_t)client->size_hints->min_width;
                } else if (event->width > client->size_hints->max_width) {
                        new_rect.width
                                = (uint16_t)client->size_hints->max_width;
                } else {
                        new_rect.width = event->width;
                }
        }

        if (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
                if (event->height < client->size_hints->min_height) {
                        new_rect.height
                                = (uint16_t)client->size_hints->min_height;
                } else if (event->height > client->size_hints->max_height) {
                        new_rect.height
                                = (uint16_t)client->size_hints->max_height;
                } else {
                        new_rect.height = event->height;
                }
        }

        client->rect = monitor_clamp_client_rect(
                monitor_get_offset_rect(monitor), new_rect);

        new_event.x = client->rect.x;
        new_event.y = client->rect.y;
        new_event.width = client->rect.width;
        new_event.height = client->rect.height;

        if (event->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
                if (event->sibling != XCB_NONE) {
                        LOG_WARNING(natwm_logger,
                                    "Specifying stacking order with sibling is "
                                    "not supported yet - Unfortunate behavior "
                                    "may occur.");
                }

                if (event->stack_mode == XCB_STACK_MODE_ABOVE) {
                        workspace_focus_client(state, workspace, client);

                } else if (event->stack_mode == XCB_STACK_MODE_BELOW) {
                        workspace_unfocus_client(state, workspace, client);
                } else {
                        // TODO: Support XCB_STACK_MODE_{OPPOSITE, TOP_IF,
                        // BOTTOM_IF}
                        LOG_WARNING(natwm_logger,
                                    "Encountered unsupported stacking order - "
                                    "Ignoring");
                }
        }

        handle_configure_request(state->xcb, &new_event);

handle_not_registered:
        handle_configure_request(state->xcb, event);

        return NO_ERROR;
}

void client_configure_window_rect(xcb_connection_t *connection,
                                  xcb_window_t window, xcb_rectangle_t rect,
                                  uint32_t border_width)
{
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
                | XCB_CONFIG_WINDOW_BORDER_WIDTH;
        uint32_t values[] = {
                (uint16_t)rect.x,
                (uint16_t)rect.y,
                rect.width,
                rect.height,
                border_width,
        };

        xcb_configure_window(connection, window, mask, values);

        xcb_flush(connection);
}

void client_map(const struct natwm_state *state, struct client *client,
                xcb_rectangle_t monitor_rect)
{
        if (client == NULL) {
                // Skip this client
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;
        uint32_t border_width = client_get_active_border_width(theme, client);

        if (client->is_fullscreen) {
                client_configure_window_rect(
                        state->xcb, client->window, monitor_rect, border_width);
        } else {
                xcb_rectangle_t new_rect = {
                        (int16_t)(client->rect.x + monitor_rect.x),
                        (int16_t)(client->rect.y + monitor_rect.y),
                        client->rect.width,
                        client->rect.height,
                };

                client_configure_window_rect(
                        state->xcb, client->window, new_rect, border_width);
        }

        if (client->state & CLIENT_HIDDEN) {
                client->state &= (uint8_t)~CLIENT_HIDDEN;
        }

        xcb_map_window(state->xcb, client->window);
}

enum natwm_error client_handle_map_notify(const struct natwm_state *state,
                                          xcb_window_t window)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, window);

        if (workspace == NULL) {
                // We do not have this window in our registry
                return NO_ERROR;
        }

        struct client *client = workspace_find_window_client(workspace, window);

        if (client == NULL) {
                return NOT_FOUND_ERROR;
        }

        if (!(client->state & CLIENT_OFF_SCREEN) || !workspace->is_visible) {
                return NO_ERROR;
        }

        if (client->is_focused && workspace->is_focused) {
                // We need to get the previous border_width before removing
                // CLIENT_OFF_SCREEN
                uint16_t border_width = client_get_active_border_width(
                        state->workspace_list->theme, client);

                client->state &= (uint8_t)~CLIENT_OFF_SCREEN;

                update_theme(state, client, border_width);
        } else {
                client->state &= (uint8_t)~CLIENT_OFF_SCREEN;
        }

        return NO_ERROR;
}

enum natwm_error client_unmap_window(struct natwm_state *state,
                                     xcb_window_t window)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, window);

        if (workspace == NULL) {
                // We do not have this window in our registry
                return NO_ERROR;
        }

        struct client *client = workspace_find_window_client(workspace, window);

        if (client == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to find registered client during unmap");

                return NOT_FOUND_ERROR;
        }

        if (!(client->state & CLIENT_OFF_SCREEN)) {
                client->state |= CLIENT_HIDDEN;
        }

        workspace_reset_focus(state, workspace);

        return NO_ERROR;
}

enum natwm_error client_destroy_window(struct natwm_state *state,
                                       xcb_window_t window)
{
        struct workspace *workspace = workspace_list_find_window_workspace(
                state->workspace_list, window);

        if (workspace == NULL) {
                // This window is not registered with us
                struct workspace *active_workspace
                        = workspace_list_get_focused(state->workspace_list);

                workspace_reset_focus(state, active_workspace);

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

xcb_rectangle_t client_initialize_rect(const struct client *client,
                                       uint16_t border_width,
                                       xcb_rectangle_t monitor_rect)
{
        xcb_rectangle_t new_rect = client->rect;

        if (client->size_hints->flags & XCB_ICCCM_SIZE_HINT_US_SIZE) {
                assert(client->size_hints->x <= INT16_MAX);
                assert(client->size_hints->y <= INT16_MAX);

                new_rect.x = (int16_t)client->size_hints->x;
                new_rect.y = (int16_t)client->size_hints->y;
        }

        if (client->size_hints->flags & XCB_ICCCM_SIZE_HINT_P_SIZE) {
                assert(client->size_hints->width <= UINT16_MAX);
                assert(client->size_hints->height <= UINT16_MAX);

                new_rect.width = (uint16_t)client->size_hints->width;
                new_rect.height = (uint16_t)client->size_hints->height;
        }

        new_rect = monitor_clamp_client_rect(monitor_rect, new_rect);

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
        if (client->is_fullscreen) {
                return 0;
        }

        if (client->state & CLIENT_URGENT) {
                return theme->border_width->urgent;
        }

        if (client->state & CLIENT_STICKY) {
                return theme->border_width->sticky;
        }

        if (client->state & CLIENT_OFF_SCREEN) {
                return theme->border_width->unfocused;
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
        if (client->state & CLIENT_URGENT) {
                return theme->color->urgent;
        }

        if (client->state & CLIENT_STICKY) {
                return theme->color->sticky;
        }

        if (client->state & CLIENT_OFF_SCREEN) {
                return theme->color->unfocused;
        }

        if (client->is_focused) {
                return theme->color->focused;
        }

        return theme->color->unfocused;
}

enum natwm_error client_set_fullscreen(const struct natwm_state *state,
                                       struct client *client)
{
        struct workspace *workspace = workspace_list_find_client_workspace(
                state->workspace_list, client);
        struct monitor *monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, workspace);

        if (monitor == NULL) {
                return RESOLUTION_FAILURE;
        }

        client->is_fullscreen = true;

        ewmh_add_window_state(
                state, client->window, state->ewmh->_NET_WM_STATE_FULLSCREEN);

        client_configure_window_rect(
                state->xcb, client->window, monitor->rect, 0);

        return NO_ERROR;
}

enum natwm_error client_unset_fullscreen(const struct natwm_state *state,
                                         struct client *client)
{
        struct client_theme *theme = state->workspace_list->theme;
        uint16_t border_width = client_get_active_border_width(theme, client);

        client->is_fullscreen = false;

        ewmh_remove_window_state(state, client->window);

        client_configure_window_rect(
                state->xcb, client->window, client->rect, border_width);

        update_theme(state, client, border_width);

        return NO_ERROR;
}

enum natwm_error
client_handle_fullscreen_window(struct natwm_state *state,
                                xcb_ewmh_wm_state_action_t action,
                                xcb_window_t window)
{
        struct client *client = workspace_list_find_window_client(
                state->workspace_list, window);

        if (client == NULL) {
                return NOT_FOUND_ERROR;
        }

        switch (action) {
        case XCB_EWMH_WM_STATE_ADD:
                return client_set_fullscreen(state, client);
        case XCB_EWMH_WM_STATE_REMOVE:
                return client_unset_fullscreen(state, client);
        case XCB_EWMH_WM_STATE_TOGGLE:
                return (client->is_fullscreen)
                        ? client_unset_fullscreen(state, client)
                        : client_set_fullscreen(state, client);
        }

        return GENERIC_ERROR;
}

void client_set_window_input_focus(const struct natwm_state *state,
                                   xcb_window_t window)
{
        ewmh_update_active_window(state, window);

        xcb_set_input_focus(state->xcb,
                            XCB_INPUT_FOCUS_POINTER_ROOT,
                            window,
                            XCB_TIME_CURRENT_TIME);

        update_stack_mode(state, window, XCB_STACK_MODE_ABOVE);
}

void client_set_focused(struct natwm_state *state, struct client *client)
{
        if (client == NULL || client->state & CLIENT_HIDDEN) {
                return;
        }

        struct workspace *workspace = workspace_list_find_client_workspace(
                state->workspace_list, client);

        if (workspace == NULL) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->is_focused = true;

        client_set_window_input_focus(state, client->window);

        if (!workspace->is_focused) {
                workspace_focus(state, workspace);
        }

        // Now that we have focused the client, there is no need for "click to
        // focus" so we can remove the button grab
        xcb_ungrab_button(state->xcb,
                          client_focus_event.button,
                          client->window,
                          client_focus_event.modifiers);

        update_theme(state, client, theme->border_width->unfocused);
}

void client_set_unfocused(const struct natwm_state *state,
                          struct client *client)
{
        if (client == NULL || client->state & CLIENT_HIDDEN) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->is_focused = false;

        // When a client is unfocused we need to grab the mouse button required
        // for "click to focus"
        mouse_event_grab_button(
                state->xcb, client->window, &client_focus_event);

        update_theme(state, client, theme->border_width->focused);
}

enum natwm_error client_focus_window(struct natwm_state *state,
                                     xcb_window_t window)
{
        struct client *client = workspace_list_find_window_client(
                state->workspace_list, window);

        if (client == NULL) {
                client_set_window_input_focus(state, window);

                return NO_ERROR;
        }

        client_set_focused(state, client);

        return NO_ERROR;
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
        free(client->size_hints);

        free(client);

        client = NULL;
}
