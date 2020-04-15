// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include <common/constants.h>
#include <common/logger.h>

#include <core/workspace.h>

#include "client.h"

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

static xcb_window_t create_parent_window(const struct natwm_state *state,
                                         xcb_rectangle_t rect,
                                         const struct client_theme *theme)
{
        // Create the parent which will contain the window decorations
        xcb_window_t parent = xcb_generate_id(state->xcb);
        uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_SAVE_UNDER;
        uint32_t values[] = {
                theme->color->unfocused->color_value,
                1,
        };

        xcb_create_window(state->xcb,
                          XCB_COPY_FROM_PARENT,
                          parent,
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

        return parent;
}

static void update_theme(xcb_connection_t *connection,
                         const struct client *client, uint16_t border_width,
                         uint32_t border_color)
{
        uint32_t values[] = {
                border_width,
        };

        xcb_configure_window(connection,
                             client->parent,
                             XCB_CONFIG_WINDOW_BORDER_WIDTH,
                             values);

        if (border_width > 0) {
                xcb_change_window_attributes(connection,
                                             client->parent,
                                             XCB_CW_BORDER_PIXEL,
                                             &border_color);
        }
}

struct client *client_create(xcb_window_t window, xcb_rectangle_t rect)
{
        struct client *client = malloc(sizeof(struct client));

        if (client == NULL) {
                return NULL;
        }

        client->child = window;
        client->rect = rect;
        client->state = CLIENT_UNFOCUSED;

        return client;
}

struct client *client_register_window(struct natwm_state *state,
                                      xcb_window_t window)
{
        struct workspace *focused_workspace
                = workspace_list_get_focused(state->workspace_list);

        if (focused_workspace == NULL) {
                return NULL;
        }

        // First get the rect for the window
        xcb_rectangle_t rect = {0};

        if (get_window_rect(state->xcb, window, &rect) != NO_ERROR) {
                return NULL;
        }

        struct client *client = client_create(window, rect);

        if (client == NULL) {
                return NULL;
        }

        // Load the client theme from the workspace list
        struct client_theme *theme = state->workspace_list->theme;
        xcb_window_t parent_window
                = create_parent_window(state, client->rect, theme);

        client->parent = parent_window;

        workspace_add_client(state, client);

        xcb_reparent_window(state->xcb, client->child, client->parent, 0, 0);

        if (focused_workspace->is_visible) {
                xcb_map_window(state->xcb, client->parent);
                xcb_map_window(state->xcb, client->child);
        }

        xcb_flush(state->xcb);

        return client;
}

void client_set_focused(const struct natwm_state *state, struct client *client)
{
        if (client == NULL || client->state == CLIENT_FOCUSED) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->state = CLIENT_FOCUSED;

        update_theme(state->xcb,
                     client,
                     theme->border_width->focused,
                     theme->color->focused->color_value);

        xcb_flush(state->xcb);
}

void client_set_unfocused(const struct natwm_state *state,
                          struct client *client)
{
        if (client == NULL || client->state == CLIENT_UNFOCUSED) {
                return;
        }

        struct client_theme *theme = state->workspace_list->theme;

        client->state = CLIENT_UNFOCUSED;

        update_theme(state->xcb,
                     client,
                     theme->border_width->unfocused,
                     theme->color->unfocused->color_value);

        xcb_flush(state->xcb);
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
