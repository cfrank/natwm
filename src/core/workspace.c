// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/logger.h>

#include "config/config.h"
#include "ewmh.h"
#include "monitor.h"
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

static size_t get_client_list_key_size(const void *window)
{
        UNUSED_FUNCTION_PARAM(window);

        return sizeof(xcb_window_t *);
}

static bool compare_windows(const void *one, const void *two, size_t key_size)
{
        UNUSED_FUNCTION_PARAM(key_size);

        xcb_window_t window_one = *(xcb_window_t *)one;
        xcb_window_t window_two = *(xcb_window_t *)two;

        return window_one == window_two;
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
                goto create_default_named_workspace;
        }

        const struct config_value *name_value = workspace_names->values[index];

        if (name_value == NULL || name_value->type != STRING) {
                LOG_WARNING(
                        natwm_logger, "Ignoring invalid workspace name", name);

                goto create_default_named_workspace;
        }

        if (strlen(name_value->data.string) > NATWM_WORKSPACE_NAME_MAX_LEN) {
                LOG_WARNING(
                        natwm_logger,
                        "Workspace name '%s' is too long. Max length is %zu",
                        name_value->data.string,
                        NATWM_WORKSPACE_NAME_MAX_LEN);

                goto create_default_named_workspace;
        }

        return workspace_create(name_value->data.string, index);

create_default_named_workspace:
        // We couldn't find a valid user specified workspace name - fall back
        // to using one from DEFAULT_WORKSPACE_NAMES
        return workspace_create(name, index);
}

struct workspace_list *workspace_list_create(size_t count)
{
        struct workspace_list *workspace_list
                = malloc(sizeof(struct workspace_list));

        if (workspace_list == NULL) {
                return NULL;
        }

        workspace_list->count = count;
        workspace_list->theme = NULL;
        workspace_list->client_map = map_init();

        map_set_key_compare_function(workspace_list->client_map,
                                     compare_windows);
        map_set_key_size_function(workspace_list->client_map,
                                  get_client_list_key_size);

        workspace_list->workspaces = calloc(count, sizeof(struct workspace *));

        if (workspace_list->workspaces == NULL) {
                free(workspace_list);

                return NULL;
        }

        return workspace_list;
}

struct workspace *workspace_create(const char *name, size_t index)
{
        struct workspace *workspace = malloc(sizeof(struct workspace));

        if (workspace == NULL) {
                return NULL;
        }

        workspace->name = name;
        workspace->index = index;
        workspace->is_visible = false;
        workspace->is_focused = false;
        workspace->clients = list_create();
        workspace->active_client = NULL;

        if (workspace->clients == NULL) {
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
                        workspace_list_destroy(state, workspace_list);

                        return MEMORY_ALLOCATION_ERROR;
                }

                workspace_list->workspaces[i] = workspace;
        }

        attach_to_monitors(state->monitor_list, workspace_list);

        ewmh_update_current_desktop(state, workspace_list->active_index);
        ewmh_update_desktop_names(state, workspace_list);

        *result = workspace_list;

        return NO_ERROR;
}

enum natwm_error workspace_add_client(struct natwm_state *state,
                                      struct workspace *workspace,
                                      struct client *client)
{
        struct workspace_list *list = state->workspace_list;
        struct client *previous_focused_client = workspace->active_client;

        // We will be modifying the state - need to lock until done
        natwm_state_lock(state);

        if (list_insert(workspace->clients, client) == NULL) {
                natwm_state_unlock(state);

                return MEMORY_ALLOCATION_ERROR;
        }

        // Cache which workspace this client is currenty active on
        map_insert(list->client_map, &client->window, &workspace->index);

        workspace->active_client = client;

        natwm_state_unlock(state);

        if (previous_focused_client) {
                client_set_unfocused(state, previous_focused_client);
        }

        client_set_focused(state, client);

        return NO_ERROR;
}

struct client *workspace_find_window_client(const struct workspace *workspace,
                                            xcb_window_t window)
{
        LIST_FOR_EACH(workspace->clients, client_item)
        {
                struct client *client = (struct client *)client_item->data;

                if (client->window == window) {
                        return client;
                }
        }

        return NULL;
}

enum natwm_error workspace_update_focused(const struct natwm_state *state,
                                          struct workspace *workspace)
{
        if (!workspace->is_visible) {
                // If the workspace is not visible there is no need to update
                // the focused client.

                return NO_ERROR;
        }

        LIST_FOR_EACH(workspace->clients, client_item)
        {
                struct client *client = (struct client *)client_item->data;

                if (client->state == CLIENT_HIDDEN) {
                        continue;
                }

                if (client->is_focused) {
                        return NO_ERROR;
                }

                // We found a visible client which needs to be focused
                workspace->active_client = client;

                client_set_focused(state, client);

                return NO_ERROR;
        }

        return NOT_FOUND_ERROR;
}

struct workspace *workspace_list_get_focused(const struct workspace_list *list)
{
        return list->workspaces[list->active_index];
}

struct workspace *
workspace_list_find_window_workspace(const struct workspace_list *list,
                                     xcb_window_t window)
{
        struct map_entry *entry = map_get(list->client_map, &window);

        if (entry == NULL) {
                return NULL;
        }

        size_t index = *(size_t *)entry->value;

        assert(index < list->count);

        return list->workspaces[index];
}

struct workspace *
workspace_list_find_client_workspace(const struct workspace_list *list,
                                     const struct client *client)
{
        return workspace_list_find_window_workspace(list, client->window);
}

struct client *
workspace_list_find_window_client(const struct workspace_list *list,
                                  xcb_window_t window)
{
        for (size_t i = 0; i < list->count; ++i) {
                struct workspace *workspace = list->workspaces[i];
                struct client *client
                        = workspace_find_window_client(workspace, window);

                if (client != NULL) {
                        return client;
                }
        }

        return NULL;
}

void workspace_list_destroy(const struct natwm_state *state,
                            struct workspace_list *workspace_list)
{
        if (workspace_list->theme != NULL) {
                client_theme_destroy(workspace_list->theme);
        }

        map_destroy(workspace_list->client_map);

        for (size_t i = 0; i < workspace_list->count; ++i) {
                if (workspace_list->workspaces[i] != NULL) {
                        workspace_destroy(state, workspace_list->workspaces[i]);
                }
        }

        free(workspace_list->workspaces);
        free(workspace_list);
}

void workspace_destroy(const struct natwm_state *state,
                       struct workspace *workspace)
{
        if (workspace->clients != NULL) {
                LIST_FOR_EACH(workspace->clients, client_item)
                {
                        struct client *client
                                = (struct client *)client_item->data;

                        xcb_destroy_window(state->xcb, client->frame);

                        client_destroy(client);
                }

                list_destroy(workspace->clients);
        }

        free(workspace);
}
