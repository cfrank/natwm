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
                               struct workspace *workspace)
{
        size_t index = 0;
        LIST_FOR_EACH(monitor_list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;
                struct space *space = workspace->spaces[index];

                space->is_visible = true;
                monitor->space = space;

                ++index;
        }
}

struct workspace *workspace_create(size_t count)
{
        struct workspace *workspace = malloc(sizeof(struct workspace));

        if (workspace == NULL) {
                return NULL;
        }

        workspace->count = count;
        workspace->spaces = calloc(count, sizeof(struct space *));

        if (workspace->spaces == NULL) {
                free(workspace);

                return NULL;
        }

        return workspace;
}

struct space *space_create(const char *tag_name)
{
        struct space *space = malloc(sizeof(struct space));

        if (space == NULL) {
                return NULL;
        }

        space->tag_name = tag_name;
        space->is_visible = false;
        space->is_focused = false;
        space->is_floating = false;

        return space;
}

enum natwm_error workspace_init(const struct natwm_state *state,
                                struct workspace **result)
{
        // First get the list of workspace names
        const struct config_array *workspace_names = NULL;

        config_find_array(state->config, "workspaces", &workspace_names);

        struct workspace *workspace = workspace_create(NATWM_WORKSPACE_COUNT);

        if (workspace == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        for (size_t i = 0; i < NATWM_WORKSPACE_COUNT; ++i) {
                // We don't have a user specified tag name for this space
                if (workspace_names == NULL || i >= workspace_names->length) {
                        workspace->spaces[i] = space_create(NULL);

                        if (workspace->spaces[i] == NULL) {
                                workspace_destroy(workspace);

                                return MEMORY_ALLOCATION_ERROR;
                        }

                        continue;
                }

                // We should have a user specified tag name for this space. Make
                // sure that it is valdi
                const struct config_value *name_value
                        = workspace_names->values[i];

                if (name_value == NULL || name_value->type != STRING) {
                        LOG_ERROR(natwm_logger,
                                  "Encountered invalid workspace tag name");

                        workspace_destroy(workspace);

                        return INVALID_INPUT_ERROR;
                }

                // We have a valid tag name for this space
                struct space *space = space_create(name_value->data.string);

                if (space == NULL) {
                        workspace_destroy(workspace);

                        return MEMORY_ALLOCATION_ERROR;
                }

                workspace->spaces[i] = space;
        }

        attach_to_monitors(state->monitor_list, workspace);

        *result = workspace;

        return NO_ERROR;
}

void workspace_destroy(struct workspace *workspace)
{
        for (size_t i = 0; i < workspace->count; ++i) {
                if (workspace->spaces[i] != NULL) {
                        space_destroy(workspace->spaces[i]);
                }
        }

        free(workspace->spaces);
        free(workspace);
}

void space_destroy(struct space *space)
{
        free(space);
}
