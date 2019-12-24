// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>

#include <common/logger.h>

#include "config/config.h"
#include "monitor.h"
#include "workspace.h"

/**
 * On first load we should give each monitor a workspace. The ordering of the
 * monitors is based on the order we recieve information about them in the
 * initial monitor setup routine.
 *
 * FIXME: In the future we should determine a better ordering of the monitors
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

                // FIXME: Maybe use the atom_name_name instead of ID which is
                // not very useful
                LOG_INFO(natwm_logger,
                         "Adding space '%s' to monitor '%u'",
                         space->tag_name,
                         monitor->id);

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
        enum natwm_error err = GENERIC_ERROR;

        // First get the list of workspace names
        const struct config_array *workspace_names = NULL;

        err = config_find_array(state->config, "workspaces", &workspace_names);

        if (err != NO_ERROR) {
                if (err == INVALID_INPUT_ERROR) {
                        LOG_ERROR(natwm_logger,
                                  "'workspaces' config item is an invalid "
                                  "type - Should be an array of strings");
                } else {
                        LOG_ERROR(natwm_logger,
                                  "Missing 'workspaces' config item");
                }

                return err;
        }

        // FIXME: This should not be needed. Users with more connected monitors
        // than workspaces should be provided workspaces with empty tag_names
        if (workspace_names->length < state->monitor_list->monitors->size) {
                LOG_ERROR(natwm_logger,
                          "There are more connected monitors than workspaces");

                return GENERIC_ERROR;
        }

        struct workspace *workspace = workspace_create(workspace_names->length);

        if (workspace == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        for (size_t i = 0; i < workspace_names->length; ++i) {
                const struct config_value *name_value
                        = workspace_names->values[i];

                if (name_value->type != STRING) {
                        LOG_ERROR(natwm_logger,
                                  "Workspace names must all be strings");

                        workspace_destroy(workspace);

                        return INVALID_INPUT_ERROR;
                }

                const char *tag_name = name_value->data.string;

                assert(tag_name);

                struct space *space = space_create(tag_name);

                LOG_INFO(natwm_logger, "%zu", i);
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
