// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include <common/constants.h>
#include <common/logger.h>

#include <core/config/config.h>

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

static enum natwm_error tile_init(const struct natwm_state *state,
                                  const struct workspace *workspace,
                                  struct tile *tile)
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

        if (get_next_tiled_rect(state, workspace, &tiled_rect) != NO_ERROR) {
                return RESOLUTION_FAILURE;
        }

        tile->tiled_rect = tiled_rect;
        tile->floating_rect = floating_rect;

        struct tile_settings_cache *settings_cache
                = state->workspace_list->settings;
        // Create window - this will be the parent window of the client
        xcb_window_t window = xcb_generate_id(state->xcb);
        uint32_t mask
                = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_SAVE_UNDER;
        uint32_t values[] = {
                settings_cache->unfocused_background_color->color_value,
                settings_cache->unfocused_border_color->color_value,
                1,
        };

        xcb_create_window(state->xcb,
                          XCB_COPY_FROM_PARENT,
                          window,
                          state->screen->root,
                          tiled_rect.x,
                          tiled_rect.y,
                          tiled_rect.width,
                          tiled_rect.height,
                          settings_cache->unfocused_border_width,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          state->screen->root_visual,
                          mask,
                          values);

        tile->parent_window = window;

        if (workspace->is_visible) {
                xcb_map_window(state->xcb, window);
        }

        return NO_ERROR;
}

struct tile *tile_create(xcb_window_t *client)
{
        struct tile *tile = malloc(sizeof(struct tile));

        if (tile == NULL) {
                return NULL;
        }

        tile->client = client;
        tile->is_floating = false;
        tile->state = TILE_UNFOCUSED;

        return tile;
}

enum natwm_error get_next_tiled_rect(const struct natwm_state *state,
                                     const struct workspace *workspace,
                                     xcb_rectangle_t *result)
{
        xcb_rectangle_t monitor_rect = {
                .x = 0,
                .y = 0,
                .width = 0,
                .height = 0,
        };

        struct monitor *workspace_monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, workspace);

        if (workspace_monitor != NULL) {
                monitor_rect = workspace_monitor->rect;
        }

        // TODO: Need to support getting rects from workspaces with more than
        // one tile
        struct tile_settings_cache *settings = state->workspace_list->settings;
        uint16_t border_width = settings->unfocused_border_width;
        uint32_t double_border_width = (uint32_t)(2 * border_width);
        uint16_t width = (uint16_t)(monitor_rect.width - double_border_width);
        uint16_t height = (uint16_t)(monitor_rect.height - double_border_width);

        xcb_rectangle_t rect = {
                .x = monitor_rect.x,
                .y = monitor_rect.y,
                .width = width,
                .height = height,
        };

        *result = rect;

        return NO_ERROR;
}

struct tile *tile_register_client(const struct natwm_state *state,
                                  xcb_window_t *client)
{
        struct workspace *focused_workspace
                = workspace_list_get_focused(state->workspace_list);

        if (focused_workspace == NULL) {
                return NULL;
        }

        struct tile *tile = focused_workspace->active_tile;

        if (tile == NULL) {
                return NULL;
        }

        tile->client = client;

        // Get the initial floating rect for the client

        if (get_window_rect(state->xcb, *tile->client, &tile->floating_rect)
            != NO_ERROR) {
                return NULL;
        }

        // Create the client
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t values[] = {
                (uint32_t)tile->tiled_rect.x,
                (uint32_t)tile->tiled_rect.y,
                tile->tiled_rect.width,
                tile->tiled_rect.height,
        };

        xcb_configure_window(state->xcb, *tile->client, mask, values);
        xcb_reparent_window(
                state->xcb, *tile->client, tile->parent_window, 0, 0);

        if (focused_workspace->is_visible) {
                xcb_map_window(state->xcb, *tile->client);
        }

        xcb_flush(state->xcb);

        return tile;
}

enum natwm_error attach_tiles_to_workspace(const struct natwm_state *state)
{
        for (size_t i = 0; i < state->workspace_list->count; ++i) {
                struct workspace *workspace
                        = state->workspace_list->workspaces[i];
                struct tile *tile = tile_create(NULL);

                if (tile == NULL) {
                        return MEMORY_ALLOCATION_ERROR;
                }

                enum natwm_error err = tile_init(state, workspace, tile);

                if (err != NO_ERROR) {
                        tile_destroy(tile);

                        return err;
                }

                tree_insert(workspace->tiles, NULL, tile);

                // Set this tile as active - being active doesn't mean it's
                // the focused tile. The only way to find the focused tile
                // if by finding the focused workspace and it's active tile
                workspace->active_tile = tile;
        }

        return NO_ERROR;
}

enum natwm_error tile_settings_cache_init(const struct map *config_map,
                                          struct tile_settings_cache **result)
{
        struct tile_settings_cache *cache
                = calloc(1, sizeof(struct tile_settings_cache));

        if (cache == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        enum natwm_error err = GENERIC_ERROR;

        uint16_t unfocused_border_width = (uint16_t)config_find_number_fallback(
                config_map, UNFOCUSED_BORDER_WIDTH_CONFIG_STRING, 1);
        uint16_t focused_border_width = (uint16_t)config_find_number_fallback(
                config_map, FOCUSED_BORDER_WIDTH_CONFIG_STRING, 1);
        uint16_t urgent_border_width = (uint16_t)config_find_number_fallback(
                config_map, URGENT_BORDER_WIDTH_CONFIG_STRING, 1);
        uint16_t sticky_border_width = (uint16_t)config_find_number_fallback(
                config_map, STICKY_BORDER_WIDTH_CONFIG_STRING, 1);

        struct color_value *unfocused_border_color = NULL;
        struct color_value *focused_border_color = NULL;
        struct color_value *urgent_border_color = NULL;
        struct color_value *sticky_border_color = NULL;

        struct color_value *unfocused_background_color = NULL;
        struct color_value *focused_background_color = NULL;
        struct color_value *urgent_background_color = NULL;
        struct color_value *sticky_background_color = NULL;

        err = color_value_from_config(config_map,
                                      UNFOCUSED_BORDER_COLOR_CONFIG_STRING,
                                      &unfocused_border_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      FOCUSED_BORDER_COLOR_CONFIG_STRING,
                                      &focused_border_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      URGENT_BORDER_COLOR_CONFIG_STRING,
                                      &urgent_border_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      STICKY_BORDER_COLOR_CONFIG_STRING,
                                      &sticky_border_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      UNFOCUSED_BACKGROUND_COLOR_CONFIG_STRING,
                                      &unfocused_background_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      FOCUSED_BACKGROUND_COLOR_CONFIG_STRING,
                                      &focused_background_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      URGENT_BACKGROUND_COLOR_CONFIG_STRING,
                                      &urgent_background_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        err = color_value_from_config(config_map,
                                      STICKY_BACKGROUND_COLOR_CONFIG_STRING,
                                      &sticky_background_color);

        if (err != NO_ERROR) {
                goto handle_error;
        }

        cache->unfocused_border_width = unfocused_border_width;
        cache->focused_border_width = focused_border_width;
        cache->urgent_border_width = urgent_border_width;
        cache->sticky_border_width = sticky_border_width;

        cache->unfocused_border_color = unfocused_border_color;
        cache->focused_border_color = focused_border_color;
        cache->urgent_border_color = urgent_border_color;
        cache->sticky_border_color = sticky_border_color;

        cache->unfocused_background_color = unfocused_background_color;
        cache->focused_background_color = focused_background_color;
        cache->urgent_background_color = urgent_background_color;
        cache->sticky_background_color = sticky_background_color;

        *result = cache;

        return NO_ERROR;

handle_error:
        tile_settings_cache_destroy(cache);

        LOG_ERROR(natwm_logger, "Failed to setup tile settings cache");

        return INVALID_INPUT_ERROR;
}

void tile_settings_cache_destroy(struct tile_settings_cache *cache)
{
        if (cache->unfocused_border_color != NULL) {
                color_value_destroy(cache->unfocused_border_color);
        }

        if (cache->focused_border_color != NULL) {
                color_value_destroy(cache->focused_border_color);
        }

        if (cache->urgent_border_color != NULL) {
                color_value_destroy(cache->urgent_border_color);
        }

        if (cache->sticky_border_color != NULL) {
                color_value_destroy(cache->sticky_border_color);
        }

        if (cache->unfocused_background_color != NULL) {
                color_value_destroy(cache->unfocused_background_color);
        }

        if (cache->focused_background_color != NULL) {
                color_value_destroy(cache->focused_background_color);
        }

        if (cache->urgent_background_color != NULL) {
                color_value_destroy(cache->urgent_background_color);
        }

        if (cache->sticky_background_color != NULL) {
                color_value_destroy(cache->sticky_background_color);
        }

        free(cache);
}

void tile_destroy(struct tile *tile)
{
        free(tile);
}
