// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/randr.h>
#include <xcb/xinerama.h>

#include <common/error.h>
#include <common/logger.h>

#include "monitor.h"
#include "randr.h"
#include "xinerama.h"

static enum server_extension
detect_server_extension(xcb_connection_t *connection)
{
        const xcb_query_extension_reply_t *cache = XCB_NONE;

        cache = xcb_get_extension_data(connection, &xcb_randr_id);

        if (cache && cache->present) {
                return RANDR;
        }

        cache = xcb_get_extension_data(connection, &xcb_xinerama_id);

        if ((cache && cache->present) && xinerama_is_active(connection)) {
                return XINERAMA;
        }

        return NO_EXTENSION;
}

static enum natwm_error monitors_from_randr(const struct natwm_state *state,
                                            struct list **result,
                                            size_t *length)
{
        struct list *monitor_list = create_list();

        if (monitor_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        size_t monitor_length = 0;
        struct randr_monitor **monitors = NULL;
        enum natwm_error err
                = randr_get_screens(state, &monitors, &monitor_length);

        if (err != NO_ERROR) {
                return err;
        }

        for (size_t i = 0; i < monitor_length; ++i) {
                struct randr_monitor *randr_monitor = monitors[i];

                if (!randr_monitor) {
                        continue;
                }

                struct monitor *monitor = monitor_create(
                        randr_monitor->id, RANDR, randr_monitor->rect, NULL);

                if (monitor == NULL) {
                        monitor_list_destroy(monitor_list);

                        return MEMORY_ALLOCATION_ERROR;
                }

                list_insert(monitor_list, monitor);

                randr_monitor_destroy(randr_monitor);
        }

        free(monitors);

        *result = monitor_list;
        *length = monitor_length;

        return NO_ERROR;
}

static enum natwm_error monitor_from_xinerama(const struct natwm_state *state,
                                              struct list **result,
                                              size_t *length)
{
        struct list *monitor_list = create_list();

        if (monitor_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        xcb_rectangle_t *rects = NULL;
        size_t monitor_length = 0;
        enum natwm_error err
                = xinerama_get_screens(state, &rects, &monitor_length);

        if (err != NO_ERROR) {
                return err;
        }

        for (size_t i = 0; i < monitor_length; ++i) {
                struct monitor *monitor
                        = monitor_create((uint32_t)i, XINERAMA, rects[i], NULL);

                if (monitor == NULL) {
                        monitor_list_destroy(monitor_list);

                        return MEMORY_ALLOCATION_ERROR;
                }

                list_insert(monitor_list, monitor);
        }

        *result = monitor_list;
        *length = monitor_length;

        return NO_ERROR;
}

static enum natwm_error monitor_from_x(const struct natwm_state *state,
                                       struct list **result, size_t *length)
{
        struct list *monitor_list = create_list();
        size_t monitor_length = 1;

        if (monitor_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        xcb_rectangle_t rect = {
                .x = 0,
                .y = 0,
                .width = state->screen->width_in_pixels,
                .height = state->screen->height_in_pixels,
        };

        struct monitor *monitor = monitor_create(0, NO_EXTENSION, rect, NULL);

        if (monitor == NULL) {
                destroy_list(monitor_list);

                return MEMORY_ALLOCATION_ERROR;
        }

        list_insert(monitor_list, monitor);

        *result = monitor_list;
        *length = monitor_length;

        return NO_ERROR;
}

const char *server_extension_to_string(enum server_extension extension)
{
        switch (extension) {
        case RANDR:
                return "RANDR";
        case XINERAMA:
                return "Xinerama";
        case NO_EXTENSION:
                return "X";
        default:
                return "";
        }

        return "";
}

struct monitor *monitor_create(uint32_t id, enum server_extension extension,
                               xcb_rectangle_t rect, struct space *space)
{
        struct monitor *monitor = malloc(sizeof(struct monitor));

        if (monitor == NULL) {
                return NULL;
        }

        monitor->id = id;
        monitor->extension = extension;
        monitor->rect = rect;
        monitor->space = space;

        return monitor;
}

enum natwm_error monitor_setup(const struct natwm_state *state,
                               struct list **result, size_t *length)
{
        enum server_extension supported_extension
                = detect_server_extension(state->xcb);

        // No matter which extension we are dealing with we will end up
        // with an error code, a list of rects, and a count of rects
        enum natwm_error err = GENERIC_ERROR;
        struct list *monitor_list = NULL;
        size_t monitor_length = 0;

        if (supported_extension == RANDR) {
                err = monitors_from_randr(
                        state, &monitor_list, &monitor_length);
        } else if (supported_extension == XINERAMA) {
                err = monitor_from_xinerama(
                        state, &monitor_list, &monitor_length);
        } else {
                err = monitor_from_x(state, &monitor_list, &monitor_length);
        }

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to setup %s screen(s)",
                          server_extension_to_string(supported_extension));

                return err;
        }

        if (monitor_length == 0) {
                LOG_ERROR(natwm_logger,
                          "Failed to find a %s screen",
                          server_extension_to_string(supported_extension));

                return INVALID_INPUT_ERROR;
        }

        *result = monitor_list;
        *length = monitor_length;

        return NO_ERROR;
}

void monitor_destroy(struct monitor *monitor)
{
        if (monitor->space != NULL) {
                space_destroy(monitor->space);
        }

        free(monitor);
}

void monitor_list_destroy(struct list *monitor_list)
{
        LIST_FOR_EACH(monitor_list, item)
        {
                struct monitor *monitor = (struct monitor *)item->data;

                monitor_destroy(monitor);
        }

        destroy_list(monitor_list);
}
