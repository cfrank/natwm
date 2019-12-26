// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>

#include <common/constants.h>
#include <common/logger.h>

#include "client.h"

struct client *client_create(xcb_window_t window, xcb_rectangle_t floating_rect)
{
        struct client *client = malloc(sizeof(struct client));

        if (client == NULL) {
                return NULL;
        }

        // The floating rect will initially be the requested size of the window
        client->window = window;
        client->floating_rect = floating_rect;
        client->rect = EMPTY_RECT;
        client->type = CLIENT_TYPE_NORMAL;
        client->status = CLIENT_STATUS_NORMAL;
        client->is_floating = false;
        client->window = window;
        client->workspace = NULL;

        return client;
}

enum natwm_error client_register(const struct natwm_state *state,
                                 xcb_window_t window, struct client **result)
{
        // Get attributes for the window
        xcb_get_window_attributes_cookie_t attributes_cookie
                = xcb_get_window_attributes(state->xcb, window);
        xcb_get_window_attributes_reply_t *attributes_reply
                = xcb_get_window_attributes_reply(
                        state->xcb, attributes_cookie, NULL);

        if (attributes_reply == NULL) {
                return RESOLUTION_FAILURE;
        }

        // If the window has requested to not be tampered with, just ignore it
        if (attributes_reply->override_redirect) {
                free(attributes_reply);

                *result = NULL;

                return NO_ERROR;
        }

        // Get the currently focused workspace
        struct workspace *focused_workspace
                = workspace_list_get_focused(state->workspace_list);

        assert(focused_workspace);

        // Get the floating rect for the window - we will initially use the
        // size that was requested by the window
        xcb_get_geometry_cookie_t geometry_cookie
                = xcb_get_geometry(state->xcb, window);
        xcb_get_geometry_reply_t *geometry_reply
                = xcb_get_geometry_reply(state->xcb, geometry_cookie, NULL);

        if (geometry_reply == NULL) {
                free(attributes_reply);

                return RESOLUTION_FAILURE;
        }

        xcb_rectangle_t floating_rect = {
                .width = geometry_reply->width,
                .height = geometry_reply->height,
                // Resolve the positioning relative to the focused workspace
                .x = (int16_t)(focused_workspace->rect.x + geometry_reply->x),
                .y = (int16_t)(focused_workspace->rect.y + geometry_reply->y),
        };

        LOG_INFO(natwm_logger,
                 "Want to map window (%ux%u) at (%d+%d)",
                 floating_rect.width,
                 floating_rect.height,
                 floating_rect.x,
                 floating_rect.y);

        free(attributes_reply);
        free(geometry_reply);

        *result = NULL;

        return NO_ERROR;
}

void client_destroy(struct client *client)
{
        free(client);
}
