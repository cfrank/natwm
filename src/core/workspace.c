// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/logger.h>

#include "config/config.h"
#include "ewmh.h"
#include "monitor.h"
#include "tile.h"
#include "workspace.h"

static const char *DEFAULT_WORKSPACE_NAMES[10] = {
        "one",
        "two",
        "three",
        "four",
        "five",
        "six",
        "seven",
        "eight",
        "nine",
        "ten",
};

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
                        workspace_list->active_index = 0;
                        workspace->is_focused = true;
                }

                workspace->is_visible = true;
                monitor->workspace = workspace;

                ++index;
        }
}

/**
 * Given a list of workspace names attempt to find a user specified workspace
 * name and initialize a new workspace with that name. If there is no name, or
 * it is invalid then create a new workspace with the default name
 */
static struct workspace *
workspace_init(const struct config_array *workspace_names, size_t index)
{
        const char *name = DEFAULT_WORKSPACE_NAMES[index];

        if (workspace_names == NULL || index >= workspace_names->length) {
                return workspace_create(name);
        }

        const struct config_value *name_value = workspace_names->values[index];

        if (name_value == NULL || name_value->type != STRING) {
                LOG_WARNING(
                        natwm_logger, "Ignoring invalid workspace name", name);

                return workspace_create(name);
        }

        if (strlen(name_value->data.string) >= NATWM_WORKSPACE_NAME_MAX_LEN) {
                LOG_WARNING(
                        natwm_logger,
                        "Workspace name '%s' is too long. Max length is %zu",
                        name_value->data.string,
                        NATWM_WORKSPACE_NAME_MAX_LEN);

                return workspace_create(name);
        }

        return workspace_create(name_value->data.string);
}

struct workspace_list *workspace_list_create(size_t count)
{
        struct workspace_list *workspace_list
                = malloc(sizeof(struct workspace_list));

        if (workspace_list == NULL) {
                return NULL;
        }

        workspace_list->count = count;
        workspace_list->settings = NULL;
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
                struct workspace *workspace
                        = workspace_init(workspace_names, i);

                if (workspace == NULL) {
                        workspace_list_destroy(workspace_list);

                        return MEMORY_ALLOCATION_ERROR;
                }

                workspace_list->workspaces[i] = workspace;
        }

        attach_to_monitors(state->monitor_list, workspace_list);

        ewmh_update_current_desktop(state, workspace_list->active_index);

        *result = workspace_list;

        return NO_ERROR;
}

struct workspace *workspace_list_get_focused(struct workspace_list *list)
{
        for (size_t i = 0; i < list->count; ++i) {
                struct workspace *workspace = list->workspaces[i];

                if (workspace->is_focused) {
                        return workspace;
                }
        }

        return NULL;
}

void workspace_list_destroy(struct workspace_list *workspace_list)
{
        if (workspace_list->settings != NULL) {
                tile_settings_cache_destroy(workspace_list->settings);
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
