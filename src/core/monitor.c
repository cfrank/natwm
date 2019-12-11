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

static void monitors_destroy(struct list *monitors)
{
        LIST_FOR_EACH(monitors, item)
        {
                struct monitor *monitor = (struct monitor *)item->data;

                monitor_destroy(monitor);
        }
}

static enum natwm_error monitors_from_randr(const struct natwm_state *state,
                                            struct list **result)
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
                destroy_list(monitor_list);

                return err;
        }

        for (size_t i = 0; i < monitor_length; ++i) {
                struct randr_monitor *randr_monitor = monitors[i];

                // A NULL monitor is not an error, it just means RANDR reported
                // n number of monitors and some were inactive so we were left
                // with fewer. In this case just skip it.
                if (randr_monitor == NULL) {
                        continue;
                }

                struct monitor *monitor = monitor_create(
                        randr_monitor->id, randr_monitor->rect, NULL);

                if (monitor == NULL) {
                        monitors_destroy(monitor_list);
                        destroy_list(monitor_list);

                        return MEMORY_ALLOCATION_ERROR;
                }

                list_insert(monitor_list, monitor);

                randr_monitor_destroy(randr_monitor);
        }

        free(monitors);

        *result = monitor_list;

        return NO_ERROR;
}

static enum natwm_error monitors_from_xinerama(const struct natwm_state *state,
                                               struct list **result)
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
                destroy_list(monitor_list);

                return err;
        }

        for (size_t i = 0; i < monitor_length; ++i) {
                struct monitor *monitor
                        = monitor_create((uint32_t)i, rects[i], NULL);

                if (monitor == NULL) {
                        monitors_destroy(monitor_list);
                        destroy_list(monitor_list);
                        free(rects);

                        return MEMORY_ALLOCATION_ERROR;
                }

                list_insert(monitor_list, monitor);
        }

        free(rects);

        *result = monitor_list;

        return NO_ERROR;
}

static enum natwm_error monitor_from_x(const struct natwm_state *state,
                                       struct list **result)
{
        struct list *monitor_list = create_list();

        if (monitor_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        xcb_rectangle_t rect = {
                .x = 0,
                .y = 0,
                .width = state->screen->width_in_pixels,
                .height = state->screen->height_in_pixels,
        };

        struct monitor *monitor = monitor_create(0, rect, NULL);

        if (monitor == NULL) {
                destroy_list(monitor_list);

                return MEMORY_ALLOCATION_ERROR;
        }

        list_insert(monitor_list, monitor);

        *result = monitor_list;

        return NO_ERROR;
}

struct server_extension *server_extension_detect(xcb_connection_t *connection)
{
        struct server_extension *extension
                = malloc(sizeof(struct server_extension));

        if (extension == NULL) {
                return NULL;
        }

        const xcb_query_extension_reply_t *cache = XCB_NONE;

        cache = xcb_get_extension_data(connection, &xcb_randr_id);

        if (cache && cache->present) {
                extension->type = RANDR;
                extension->data_cache = cache;

                return extension;
        }

        cache = xcb_get_extension_data(connection, &xcb_xinerama_id);

        if ((cache && cache->present) && xinerama_is_active(connection)) {
                extension->type = XINERAMA;
                extension->data_cache = cache;

                return extension;
        }

        extension->type = NO_EXTENSION;
        extension->data_cache = NULL;

        return extension;
}

const char *server_extension_to_string(enum server_extension_type extension)
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

struct monitor_list *monitor_list_create(struct server_extension *extension,
                                         struct list *monitors)
{
        struct monitor_list *list = malloc(sizeof(struct monitor_list));

        if (list == NULL) {
                return NULL;
        }

        list->extension = extension;
        list->monitors = monitors;

        return list;
}

struct monitor *monitor_create(uint32_t id, xcb_rectangle_t rect,
                               struct space *space)
{
        struct monitor *monitor = malloc(sizeof(struct monitor));

        if (monitor == NULL) {
                return NULL;
        }

        monitor->id = id;
        monitor->rect = rect;
        monitor->space = space;

        return monitor;
}

enum natwm_error monitor_setup(const struct natwm_state *state,
                               struct monitor_list **result)
{
        struct server_extension *extension
                = server_extension_detect(state->xcb);

        if (extension == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        // Resolve monitors from any extension into a linked list of generic
        // monitors.
        enum natwm_error err = GENERIC_ERROR;
        struct list *monitors = NULL;

        if (extension->type == RANDR) {
                err = monitors_from_randr(state, &monitors);
        } else if (extension->type == XINERAMA) {
                err = monitors_from_xinerama(state, &monitors);
        } else {
                err = monitor_from_x(state, &monitors);
        }

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to setup %s screen(s)",
                          server_extension_to_string(extension->type));

                free(extension);

                return err;
        }

        if (monitors->size == 0) {
                LOG_ERROR(natwm_logger,
                          "Failed to find a %s screen",
                          server_extension_to_string(extension->type));

                free(extension);

                return INVALID_INPUT_ERROR;
        }

        struct monitor_list *monitor_list
                = monitor_list_create(extension, monitors);

        if (monitor_list == NULL) {
                free(extension);
                destroy_list(monitors);

                return MEMORY_ALLOCATION_ERROR;
        }

        *result = monitor_list;

        return NO_ERROR;
}

void monitor_list_destroy(struct monitor_list *monitor_list)
{
        free(monitor_list->extension);
        monitors_destroy(monitor_list->monitors);
        destroy_list(monitor_list->monitors);
        free(monitor_list);
}

void monitor_destroy(struct monitor *monitor)
{
        if (monitor->space != NULL) {
                space_destroy(monitor->space);
        }

        free(monitor);
}
