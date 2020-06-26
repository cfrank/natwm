// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>

#include <common/logger.h>

#include "randr.h"

static struct randr_monitor *randr_monitor_create(xcb_randr_crtc_t id, xcb_rectangle_t rect)
{
        struct randr_monitor *monitor = malloc(sizeof(struct randr_monitor));

        if (monitor == NULL) {
                return NULL;
        }

        monitor->id = id;
        monitor->rect = rect;

        return monitor;
}

enum natwm_error randr_get_screens(const struct natwm_state *state, struct randr_monitor ***result,
                                   size_t *length)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_randr_get_screen_resources_cookie_t resources_cookie
                = xcb_randr_get_screen_resources(state->xcb, state->screen->root);
        xcb_randr_get_screen_resources_reply_t *resources_reply
                = xcb_randr_get_screen_resources_reply(state->xcb, resources_cookie, &err);

        if (err != XCB_NONE || resources_reply == NULL) {
                LOG_ERROR(natwm_logger, "Failed to get RANDR screens");

                if (resources_reply != NULL) {
                        free(resources_reply);
                }

                return GENERIC_ERROR;
        }

        int screen_count = xcb_randr_get_screen_resources_outputs_length(resources_reply);

        assert(screen_count > 0);

        struct randr_monitor **monitors
                = calloc((size_t)screen_count, sizeof(struct randr_monitor *));

        if (monitors == NULL) {
                free(resources_reply);

                return MEMORY_ALLOCATION_ERROR;
        }

        xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_outputs(resources_reply);

        for (int i = 0; i < screen_count; ++i) {
                xcb_randr_get_output_info_cookie_t cookie
                        = xcb_randr_get_output_info(state->xcb, outputs[i], XCB_CURRENT_TIME);
                xcb_randr_get_output_info_reply_t *output_info_reply
                        = xcb_randr_get_output_info_reply(state->xcb, cookie, &err);

                if (err != XCB_NONE && output_info_reply == NULL) {
                        LOG_WARNING(natwm_logger, "Failed to get info for a RANDR screen.");

                        free(err);

                        continue;
                }

                xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info(
                        state->xcb, output_info_reply->crtc, XCB_CURRENT_TIME);
                xcb_randr_get_crtc_info_reply_t *crtc_info_reply
                        = xcb_randr_get_crtc_info_reply(state->xcb, crtc_info_cookie, &err);

                if (err != XCB_NONE && crtc_info_reply == NULL) {
                        // We encounter this when we find an inactive RANDR
                        // screens
                        free(output_info_reply);
                        free(err);

                        continue;
                }

                xcb_rectangle_t screen_rect = {
                        .x = crtc_info_reply->x,
                        .y = crtc_info_reply->y,
                        .width = crtc_info_reply->width,
                        .height = crtc_info_reply->height,
                };

                struct randr_monitor *monitor
                        = randr_monitor_create(output_info_reply->crtc, screen_rect);

                if (monitor == NULL) {
                        // Mem error, need to free existing monitors
                        for (int j = 0; j < i; ++j) {
                                if (monitors[j] != NULL) {
                                        randr_monitor_destroy(monitors[j]);
                                }
                        }

                        free(monitors);

                        return MEMORY_ALLOCATION_ERROR;
                }

                monitors[i] = monitor;

                free(crtc_info_reply);
                free(output_info_reply);
        }

        // Listen for events
        xcb_randr_select_input(
                state->xcb, state->screen->root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);

        *result = monitors;
        *length = (size_t)screen_count;

        free(resources_reply);

        return NO_ERROR;
}

void randr_monitor_destroy(struct randr_monitor *monitor)
{
        free(monitor);
}
