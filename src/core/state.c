// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "state.h"
#include "ewmh.h"

struct natwm_state *natwm_state_create(void)
{
        struct natwm_state *state = calloc(1, sizeof(struct natwm_state));

        if (state == NULL) {
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

        if (state->config != NULL) {
                config_destroy((struct map *)state->config);
        }

        free(state);
}
