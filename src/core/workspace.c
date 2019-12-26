// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include <common/constants.h>
#include <common/logger.h>

#include "config/config.h"
#include "monitor.h"
#include "workspace.h"

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

                workspace->is_visible = true;
                workspace->rect = monitor->rect;

                monitor->workspace = workspace;

                ++index;
        }
}

struct workspace_list *workspace_list_create(size_t count)
{
        struct workspace_list *workspace_list
                = malloc(sizeof(struct workspace_list));

        if (workspace_list == NULL) {
                return NULL;
        }

        workspace_list->count = count;
        workspace_list->workspaces = calloc(count, sizeof(struct workspace *));

        if (workspace_list->workspaces == NULL) {
                free(workspace_list);

                return NULL;
        }

        return workspace_list;
}

struct workspace *workspace_create(const char *name, xcb_rectangle_t rect)
{
        struct workspace *workspace = malloc(sizeof(struct workspace));

        if (workspace == NULL) {
                return NULL;
        }

        workspace->name = name;
        workspace->rect = rect;
        workspace->is_visible = false;
        workspace->is_focused = false;
        workspace->is_floating = false;

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
                // We don't have a user specified tag name for this space
                if (workspace_names == NULL || i >= workspace_names->length) {
                        workspace_list->workspaces[i]
                                = workspace_create(NULL, EMPTY_RECT);

                        if (workspace_list->workspaces[i] == NULL) {
                                workspace_list_destroy(workspace_list);

                                return MEMORY_ALLOCATION_ERROR;
                        }

                        continue;
                }

                // We should have a user specified tag name for this workspace.
                // Make sure that it is valid
                const struct config_value *name_value
                        = workspace_names->values[i];

                if (name_value == NULL || name_value->type != STRING) {
                        LOG_ERROR(natwm_logger,
                                  "Encountered invalid workspace tag name");

                        workspace_list_destroy(workspace_list);

                        return INVALID_INPUT_ERROR;
                }

                // We have a valid tag name for this workspace
                struct workspace *workspace
                        = workspace_create(name_value->data.string, EMPTY_RECT);

                if (workspace == NULL) {
                        workspace_list_destroy(workspace_list);

                        return MEMORY_ALLOCATION_ERROR;
                }

                workspace_list->workspaces[i] = workspace;
        }

        attach_to_monitors(state->monitor_list, workspace_list);

        // Focus on the first workspace
        workspace_set_focused(workspace_list, 0);

        *result = workspace_list;

        return NO_ERROR;
}

// TODO: Much more is needed for this function
void workspace_set_focused(struct workspace_list *workspace_list, size_t index)
{
        workspace_list->active_index = index;
}

struct workspace *
workspace_list_get_focused(const struct workspace_list *workspace_list)
{
        return workspace_list->workspaces[workspace_list->active_index];
}

void workspace_list_destroy(struct workspace_list *workspace_list)
{
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
        free(workspace);
}
