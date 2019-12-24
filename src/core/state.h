// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <pthread.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include <common/list.h>
#include <common/map.h>

// Forward declare needed types
struct monitor_list;
struct workspace;

struct natwm_state {
        int screen_num;
        xcb_connection_t *xcb;
        xcb_ewmh_connection_t *ewmh;
        xcb_screen_t *screen;
        struct monitor_list *monitor_list;
        struct workspace *workspace;
        const struct map *config;
        const char *config_path;
        pthread_mutex_t mutex;
};

struct natwm_state *natwm_state_create(void);
void natwm_state_destroy(struct natwm_state *state);
void natwm_state_update_config(struct natwm_state *state,
                               const struct map *new_config);
