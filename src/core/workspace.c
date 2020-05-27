// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

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

static struct client *get_client_from_client_node(struct node *client_node)
{
        return (struct client *)client_node->data;
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

static void workspace_send_to_monitor(struct natwm_state *state,
                                      struct workspace *workspace,
                                      struct monitor *monitor)
{
        struct monitor *previous_monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, workspace);

        LIST_FOR_EACH(workspace->clients, node)
        {
                struct client *client = get_client_from_client_node(node);

                if (client == NULL || client->state & CLIENT_HIDDEN) {
                        continue;
                }

                client->state &= (uint8_t)~CLIENT_OFF_SCREEN;

                // Update client size to match aspect ratio of new monitor
                client->rect = monitor_move_client_rect(
                        previous_monitor, monitor, client);

                client_map(state, client, monitor->rect);

                if (client->is_focused) {
                        client_set_window_input_focus(state, client->window);
                }
        }

        if (list_is_empty(workspace->clients)) {
                client_set_window_input_focus(state, state->screen->root);
        }

        workspace->is_visible = true;
        monitor->workspace = workspace;
        ewmh_update_current_desktop(state, workspace->index);
}

static void workspace_hide(const struct natwm_state *state,
                           struct workspace *workspace)
{
        if (!workspace->is_visible) {
                return;
        }

        workspace->is_focused = false;
        workspace->is_visible = false;

        LIST_FOR_EACH(workspace->clients, node)
        {
                struct client *client = get_client_from_client_node(node);

                if (client == NULL || client->state & CLIENT_HIDDEN) {
                        continue;
                }

                client->state |= CLIENT_OFF_SCREEN;

                xcb_unmap_window(state->xcb, client->window);
        }
}

static struct node *get_client_node_from_client(struct list *client_list,
                                                struct client *client)
{
        LIST_FOR_EACH(client_list, node)
        {
                struct client *client_item = get_client_from_client_node(node);

                if (client->window == client_item->window) {
                        return node;
                }
        }

        return NULL;
}

static void focus_client(struct natwm_state *state, struct workspace *workspace,
                         struct node *node, struct client *client)
{
        natwm_state_lock(state);

        if (workspace->active_client) {
                client_set_unfocused(state, workspace->active_client);
        }

        workspace->active_client = client;

        list_move_node_to_head(workspace->clients, node);

        client_set_focused(state, client);

        natwm_state_unlock(state);
}

// Focus on the root window when there is no other windows to focus
static void reset_input_focus(const struct natwm_state *state)
{
        ewmh_update_active_window(state, state->screen->root);

        xcb_set_input_focus(state->xcb,
                            XCB_INPUT_FOCUS_NONE,
                            state->screen->root,
                            XCB_TIME_CURRENT_TIME);
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

        if (workspace_list->client_map == NULL) {
                free(workspace_list);

                return NULL;
        }

        map_set_key_compare_function(workspace_list->client_map,
                                     compare_windows);
        map_set_key_size_function(workspace_list->client_map,
                                  get_client_list_key_size);

        workspace_list->workspaces = calloc(count, sizeof(struct workspace *));

        if (workspace_list->workspaces == NULL) {
                map_destroy(workspace_list->client_map);

                free(workspace_list);

                return NULL;
        }

        return workspace_list;
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
        ewmh_update_desktop_names(state, workspace_list);

        reset_input_focus(state);

        *result = workspace_list;

        return NO_ERROR;
}

enum natwm_error workspace_focus_existing_client(struct natwm_state *state,
                                                 struct workspace *workspace,
                                                 struct client *client)
{
        if (client->is_focused || client->state & CLIENT_HIDDEN) {
                return INVALID_INPUT_ERROR;
        }

        struct node *client_node
                = get_client_node_from_client(workspace->clients, client);

        if (client_node == NULL) {
                return NOT_FOUND_ERROR;
        }

        focus_client(state, workspace, client_node, client);

        return NO_ERROR;
}

enum natwm_error workspace_unfocus_existing_client(struct natwm_state *state,
                                                   struct workspace *workspace,
                                                   struct client *client)
{
        struct node *client_node
                = get_client_node_from_client(workspace->clients, client);

        if (client_node == NULL) {
                return NOT_FOUND_ERROR;
        } else if (client_node->next == NULL) {
                return INVALID_INPUT_ERROR;
        }

        natwm_state_lock(state);

        if (client->is_focused) {
                struct node *next_node = client_node->next;
                struct client *next_client
                        = get_client_from_client_node(next_node);

                workspace_focus_existing_client(state, workspace, next_client);
        }

        list_move_node_to_tail(workspace->clients, client_node);

        natwm_state_unlock(state);

        return NO_ERROR;
}

enum natwm_error workspace_reset_focus(struct natwm_state *state,
                                       struct workspace *workspace)
{
        LIST_FOR_EACH(workspace->clients, node)
        {
                struct client *client = get_client_from_client_node(node);

                if (client->state & CLIENT_HIDDEN) {
                        continue;
                }

                if (client->is_focused) {
                        return NO_ERROR;
                }

                focus_client(state, workspace, node, client);

                return NO_ERROR;
        }

        // There are no more clients on this workspace
        workspace->active_client = NULL;

        reset_input_focus(state);

        return NOT_FOUND_ERROR;
}

enum natwm_error workspace_change_monitor(struct natwm_state *state,
                                          struct workspace *next_workspace)
{
        if (next_workspace->is_focused) {
                return NO_ERROR;
        }

        // Hide the currently focused workspace
        struct workspace *current_workspace
                = workspace_list_get_focused(state->workspace_list);
        struct monitor *current_monitor
                = monitor_list_get_active_monitor(state->monitor_list);
        struct monitor *next_monitor = monitor_list_get_workspace_monitor(
                state->monitor_list, next_workspace);

        if (current_workspace == NULL) {
                return RESOLUTION_FAILURE;
        }

        // Hide the current workspace
        workspace_hide(state, current_workspace);

        if (next_workspace->is_visible && next_monitor) {
                workspace_hide(state, next_workspace);

                workspace_send_to_monitor(
                        state, current_workspace, next_monitor);
        }

        workspace_send_to_monitor(state, next_workspace, current_monitor);

        next_workspace->is_focused = true;

        state->workspace_list->active_index = next_workspace->index;

        return NO_ERROR;
}

// This is a more simple version of workspace_reset_focus - all it does is
// reset the input focus of the first client it can find, falling back to the
// root window.
void workspace_reset_input_focus(struct natwm_state *state,
                                 struct workspace *workspace)
{
        if (!workspace->is_visible) {
                return;
        }

        LIST_FOR_EACH(workspace->clients, node)
        {
                struct client *client = get_client_from_client_node(node);

                if (client == NULL || client->state & CLIENT_HIDDEN) {
                        continue;
                }

                client_set_window_input_focus(state, client->window);

                return;
        }

        reset_input_focus(state);
}

enum natwm_error workspace_add_client(struct natwm_state *state,
                                      struct workspace *workspace,
                                      struct client *client)
{
        struct workspace_list *list = state->workspace_list;

        // We will be modifying the state - need to lock until done
        natwm_state_lock(state);

        if (list_insert(workspace->clients, client) == NULL) {
                natwm_state_unlock(state);

                return MEMORY_ALLOCATION_ERROR;
        }

        // Cache which workspace this client is currenty active on
        map_insert(list->client_map, &client->window, &workspace->index);

        // Update active_client
        if (workspace->active_client) {
                client_set_unfocused(state, workspace->active_client);
        }

        workspace->active_client = client;

        natwm_state_unlock(state);

        client_set_focused(state, client);

        return NO_ERROR;
}

enum natwm_error workspace_remove_client(struct natwm_state *state,
                                         struct workspace *workspace,
                                         struct client *client)
{
        struct node *client_node
                = get_client_node_from_client(workspace->clients, client);

        if (client_node == NULL) {
                return NOT_FOUND_ERROR;
        }

        natwm_state_lock(state);

        list_remove(workspace->clients, client_node);

        node_destroy(client_node);

        map_delete(state->workspace_list->client_map, &client->window);

        natwm_state_unlock(state);

        if (client->is_focused) {
                workspace_reset_focus(state, workspace);
        }

        return NO_ERROR;
}

struct client *workspace_find_window_client(const struct workspace *workspace,
                                            xcb_window_t window)
{
        LIST_FOR_EACH(workspace->clients, node)
        {
                struct client *client = get_client_from_client_node(node);

                if (client == NULL) {
                        continue;
                }

                if (client->window == window) {
                        return client;
                }
        }

        return NULL;
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

enum natwm_error workspace_list_switch_to_workspace(struct natwm_state *state,
                                                    uint32_t workspace_index)
{
        if (workspace_index >= state->workspace_list->count) {
                LOG_WARNING(natwm_logger,
                            "Attempted to switch to non-existent workspace %u",
                            workspace_index);

                return INVALID_INPUT_ERROR;
        }

        struct workspace *next_workspace
                = state->workspace_list->workspaces[workspace_index];

        return workspace_change_monitor(state, next_workspace);

        return NO_ERROR;
}

void workspace_list_destroy(struct workspace_list *workspace_list)
{
        if (workspace_list->theme != NULL) {
                client_theme_destroy(workspace_list->theme);
        }

        map_destroy(workspace_list->client_map);

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
        if (workspace->clients != NULL) {
                LIST_FOR_EACH(workspace->clients, node)
                {
                        struct client *client
                                = get_client_from_client_node(node);

                        client_destroy(client);
                }

                list_destroy(workspace->clients);
        }

        free(workspace);
}
