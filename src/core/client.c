// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <common/logger.h>

#include "client.h"
#include "workspace.h"

static xcb_get_geometry_reply_t *
get_window_geometry(xcb_connection_t *connection, xcb_window_t window)
{
        xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, window);
        xcb_get_geometry_reply_t *reply
                = xcb_get_geometry_reply(connection, cookie, NULL);

        if (reply == NULL) {
                return NULL;
        }

        return reply;
}

struct client *client_create(const struct natwm_state *state,
                             xcb_window_t window, size_t space_index)
{
        struct client *client = malloc(sizeof(struct client));

        if (client == NULL) {
                return NULL;
        }

        // Get initial floating rect
        xcb_get_geometry_reply_t *window_geometry
                = get_window_geometry(state->xcb, window);

        if (window_geometry == NULL) {
                free(client);

                return NULL;
        }

        xcb_rectangle_t floating_rect = {
                .x = window_geometry->x,
                .y = window_geometry->y,
                .width = window_geometry->width,
                .height = window_geometry->height,
        };

        LOG_INFO(natwm_logger,
                 "Adding window at %d:%d with a size of %ux%u",
                 floating_rect.x,
                 floating_rect.y,
                 floating_rect.width,
                 floating_rect.height);

        client->space_index = space_index;
        client->window = window;
        client->floating_rect = floating_rect;
        client->type = NORMAL;
        client->is_sticky = false;
        client->is_fullscreen = false;
        client->is_urgent = false;

        free(window_geometry);

        return client;
}

struct client *client_register_window(const struct natwm_state *state,
                                      xcb_window_t window)
{
        const struct space *focused_space = state->workspace->focused_space;

        if (focused_space == NULL) {
                // TODO: Maybe set this instead of returning NULL
                return NULL;
        }

        LOG_INFO(natwm_logger, "Creating client");
        struct client *client
                = client_create(state, window, focused_space->index);

        return client;
}

