// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "state.h"
#include "config/config.h"
#include "ewmh.h"
#include "monitor.h"
#include "workspace.h"

struct natwm_state *natwm_state_create(void)
{
        struct natwm_state *state = calloc(1, sizeof(struct natwm_state));

        if (state == NULL) {
                return NULL;
        }

        state->screen_num = -1;
        state->xcb = NULL;
        state->ewmh = NULL;
        state->screen = NULL;
        state->monitors = NULL;
        state->workspace = NULL;
        state->config = NULL;
        state->config_path = NULL;

        // Initialize mutex
        if (pthread_mutex_init(&state->mutex, NULL) != 0) {
                return NULL;
        }

        return state;
}

void natwm_state_destroy(struct natwm_state *state)
{
        if (state == NULL) {
                return;
        }

        if (state->xcb != NULL) {
                xcb_disconnect(state->xcb);
        }

        if (state->ewmh != NULL) {
                ewmh_destroy(state->ewmh);
        }

        if (state->monitors != NULL) {
                monitor_list_destroy(state->monitors);
        }

        if (state->workspace != NULL) {
                workspace_destroy(state->workspace);
        }

        if (state->config != NULL) {
                config_destroy((struct map *)state->config);
        }

        pthread_mutex_destroy(&state->mutex);

        free(state);
}

void natwm_state_update_config(struct natwm_state *state,
                               const struct map *new_config)
{
        pthread_mutex_lock(&state->mutex);

        state->config = new_config;

        pthread_mutex_unlock(&state->mutex);
}
