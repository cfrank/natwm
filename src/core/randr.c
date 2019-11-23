// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <xcb/randr.h>

#include <common/logger.h>

#include "randr.h"

enum natwm_error randr_get_screens(const struct natwm_state *state,
                                   xcb_rectangle_t **destination, size_t *count)
{
        xcb_generic_error_t *err = XCB_NONE;

        xcb_randr_get_screen_resources_cookie_t resources_cookie
                = xcb_randr_get_screen_resources(state->xcb,
                                                 state->screen->root);
        xcb_randr_get_screen_resources_reply_t *resources_reply
                = xcb_randr_get_screen_resources_reply(
                        state->xcb, resources_cookie, &err);

        if (err != XCB_NONE || resources_reply == NULL) {
                LOG_ERROR(natwm_logger, "Failed to get randr screens");

                if (resources_reply != NULL) {
                        free(resources_reply);
                }

                return GENERIC_ERROR;
        }

        int screen_count = xcb_randr_get_screen_resources_outputs_length(
                resources_reply);

        assert(screen_count > 0);

        xcb_rectangle_t *screen_rects
                = malloc(sizeof(xcb_rectangle_t) * (size_t)screen_count);

        if (screen_rects == NULL) {
                free(resources_reply);

                return MEMORY_ALLOCATION_ERROR;
        }

        for (size_t i = 0; i < (size_t)screen_count; ++i) {
                xcb_randr_output_t *outputs
                        = xcb_randr_get_screen_resources_outputs(
                                resources_reply);
                xcb_randr_get_output_info_cookie_t cookie
                        = xcb_randr_get_output_info(
                                state->xcb, outputs[i], XCB_CURRENT_TIME);
                xcb_randr_get_output_info_reply_t *output_info_reply
                        = xcb_randr_get_output_info_reply(
                                state->xcb, cookie, &err);

                if (err != XCB_NONE || output_info_reply == NULL) {
                        LOG_WARNING(natwm_logger,
                                    "Failed to get info for a RANDR screen.");

                        continue;
                }

                xcb_randr_get_crtc_info_cookie_t crtc_info_cookie
                        = xcb_randr_get_crtc_info(state->xcb,
                                                  output_info_reply->crtc,
                                                  XCB_CURRENT_TIME);
                xcb_randr_get_crtc_info_reply_t *crtc_info_reply
                        = xcb_randr_get_crtc_info_reply(
                                state->xcb, crtc_info_cookie, &err);

                if (err != XCB_NONE || crtc_info_reply == NULL) {
                        LOG_WARNING(natwm_logger,
                                    "Failed to get info for a RANDR screen.");

                        free(output_info_reply);

                        continue;
                }

                xcb_rectangle_t screen_rect = {
                        .x = crtc_info_reply->x,
                        .y = crtc_info_reply->y,
                        .width = crtc_info_reply->width,
                        .height = crtc_info_reply->height,
                };

                screen_rects[i] = screen_rect;

                free(crtc_info_reply);
                free(output_info_reply);
        }

        *destination = screen_rects;
        *count = (size_t)screen_count;

        free(resources_reply);

        return NO_ERROR;
}
