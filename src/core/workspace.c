// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include <common/constants.h>
#include <common/logger.h>

#include "config/config.h"
#include "monitor.h"
#include "tile.h"
#include "workspace.h"

static void workspace_tiles_destroy_callback(struct leaf *leaf)
{
        if (leaf != NULL && leaf->data != NULL) {
                tile_destroy((struct tile *)leaf->data);
        }

        leaf_destroy(leaf);
}

/**
 * On first load we should give each monitor a workspace. The ordering of the
 * monitors is based on the order we recieve information about them in the
 * initial monitor setup routine.
 *
 * TODO: In the future we should determine a better ordering of the monitors
 * based on their positioning. This could also be a user configurable option.
 */
static void attach_to_monitors(struct monitor_list *monitor_list,
                               struct workspace_list *workspace_list)
{
        size_t index = 0;
        LIST_FOR_EACH(monitor_list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;
                struct workspace *workspace = workspace_list->workspaces[index];

                // Focus on the first monitor
                if (index == 0) {
                        workspace->is_focused = true;
                }

                workspace->is_visible = true;
                monitor->workspace = workspace;

                ++index;
        }
}

static enum natwm_error attach_tiles(const struct natwm_state *state,
                                     struct workspace_list *workspace_list)
{
        for (size_t i = 0; i < workspace_list->count; ++i) {
                struct tile *tile = tile_create(NULL);

                if (tile == NULL) {
                        return MEMORY_ALLOCATION_ERROR;
                }

                enum natwm_error err = tile_init(state, tile);

                if (err != NO_ERROR) {
                        tile_destroy(tile);

                        return err;
                }

                tree_insert(workspace_list->workspaces[i]->tiles, NULL, tile);

                LOG_INFO(natwm_logger, "--- Initializing tile: ---");

                LOG_INFO(natwm_logger,
                         "Floating Rect: %ux%u+%d+%d",
                         tile->floating_rect.width,
                         tile->floating_rect.height,
                         tile->floating_rect.x,
                         tile->floating_rect.y);

                LOG_INFO(natwm_logger,
                         "Tiled Rect: %ux%u+%d+%d",
                         tile->tiled_rect.width,
                         tile->tiled_rect.height,
                         tile->tiled_rect.x,
                         tile->tiled_rect.y);

                LOG_INFO(natwm_logger, "--- END ---");
        }

        return NO_ERROR;
}

struct workspace_list *workspace_list_create(size_t count)
{
        struct workspace_list *workspace_list
                = malloc(sizeof(struct workspace_list));

        if (workspace_list == NULL) {
                return NULL;
        }

        workspace_list->count = count;
        workspace_list->tile_settings_cache = NULL;
        workspace_list->workspaces = calloc(count, sizeof(struct workspace *));

        if (workspace_list->workspaces == NULL) {
                free(workspace_list);

                return NULL;
        }

        return workspace_list;
}

struct workspace *workspace_create(const char *name)
{
        struct workspace *workspace = malloc(sizeof(struct workspace));

        if (workspace == NULL) {
                return NULL;
        }

        workspace->name = name;
        workspace->is_visible = false;
        workspace->is_focused = false;
        workspace->is_floating = false;
        workspace->tiles = tree_create(NULL);

        if (workspace->tiles == NULL) {
                free(workspace);

                return NULL;
        }

        return workspace;
}

enum natwm_error workspace_list_init(const struct natwm_state *state,
                                     struct workspace_list **result)
{
        // First get the list of workspace names
        const struct config_array *workspace_names = NULL;

        config_find_array(state->config, "workspaces", &workspace_names);

        struct workspace_list *workspace_list
                = workspace_list_create(NATWM_WORKSPACE_COUNT);

        if (workspace_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        for (size_t i = 0; i < NATWM_WORKSPACE_COUNT; ++i) {
                struct workspace *workspace = NULL;

                // We don't have a user specified tag name for this space
                if (workspace_names == NULL || i >= workspace_names->length) {
                        workspace = workspace_create(NULL);

                        if (workspace == NULL) {
                                workspace_list_destroy(workspace_list);

                                return MEMORY_ALLOCATION_ERROR;
                        }
                } else {
                        const struct config_value *name_value
                                = workspace_names->values[i];

                        if (name_value == NULL || name_value->type != STRING) {
                                LOG_ERROR(natwm_logger,
                                          "Encountered invalid workspace tag "
                                          "name");

                                workspace_list_destroy(workspace_list);

                                return INVALID_INPUT_ERROR;
                        }

                        workspace = workspace_create(name_value->data.string);

                        if (workspace == NULL) {
                                workspace_list_destroy(workspace_list);

                                return MEMORY_ALLOCATION_ERROR;
                        }
                }

                workspace_list->workspaces[i] = workspace;
        }

        attach_to_monitors(state->monitor_list, workspace_list);

        // Before we start creating tiles let's make an initial settings cache
        // based on the present configuration cache.
        //
        // TODO: Set defaults so we don't have a hard requirement on the user
        // settings these in their configuration
        if (tile_settings_cache_init(state->config,
                                     &workspace_list->tile_settings_cache)
            != NO_ERROR) {
                workspace_list_destroy(workspace_list);

                return RESOLUTION_FAILURE;
        }

        if (attach_tiles(state, workspace_list) != NO_ERROR) {
                workspace_list_destroy(workspace_list);

                return RESOLUTION_FAILURE;
        }

        *result = workspace_list;

        return NO_ERROR;
}

void workspace_list_destroy(struct workspace_list *workspace_list)
{
        if (workspace_list->tile_settings_cache != NULL) {
                tile_settings_cache_destroy(
                        workspace_list->tile_settings_cache);
        }

        for (size_t i = 0; i < workspace_list->count; ++i) {
                if (workspace_list->workspaces[i] != NULL) {
                        workspace_destroy(workspace_list->workspaces[i]);
                }
        }

        free(workspace_list->workspaces);
        free(workspace_list);
}

void workspace_destroy(struct workspace *workspace)
{
        if (workspace->tiles != NULL) {
                tree_destroy(workspace->tiles,
                             workspace_tiles_destroy_callback);
        }

        free(workspace);
}
