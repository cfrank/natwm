// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include <common/constants.h>

#include "monitor.h"
#include "tile.h"

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

        return NO_ERROR;
}

struct tile *tile_create(xcb_window_t *window)
{
        struct tile *tile = malloc(sizeof(struct tile));

        if (tile == NULL) {
                return NULL;
        }

        tile->window = window;
        tile->is_floating = false;

        return tile;
}

enum natwm_error get_next_tile_rect(const struct natwm_state *state,
                                    xcb_rectangle_t *result)
{
        // Find the currently active monitor
        struct monitor *active_monitor
                = monitor_list_get_active_monitor(state->monitor_list);

        if (active_monitor == NULL) {
                return NOT_FOUND_ERROR;
        }

        // TODO: Need to support getting rects from workspaces with more than
        // one tile

        *result = active_monitor->rect;

        return NO_ERROR;
}

enum natwm_error tile_init(const struct natwm_state *state, struct tile *tile)
{
        xcb_rectangle_t tiled_rect = {
                .width = 0,
                .height = 0,
                .x = 0,
                .y = 0,
        };
        xcb_rectangle_t floating_rect = {
                .width = 0,
                .height = 0,
                .x = 0,
                .y = 0,
        };

        // TODO: We need to account for borders and gaps
        if (get_next_tile_rect(state, &tiled_rect) != NO_ERROR) {
                return RESOLUTION_FAILURE;
        }

        if (tile->window != NULL) {
                if (get_window_rect(state->xcb, *tile->window, &floating_rect)
                    != NO_ERROR) {
                        return RESOLUTION_FAILURE;
                }
        }

        tile->tiled_rect = tiled_rect;
        tile->floating_rect = floating_rect;

        return NO_ERROR;
}

void tile_destroy(struct tile *tile)
{
        free(tile);
}
