// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <math.h>
#include <string.h>
#include <xcb/randr.h>
#include <xcb/xinerama.h>

#include <common/constants.h>
#include <common/error.h>
#include <common/logger.h>
#include <common/util.h>

#include "config/config.h"
#include "ewmh.h"
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

static void monitor_list_set_offsets(const struct natwm_state *state,
                                     struct monitor_list *monitor_list)
{
        const struct config_array *offset_array = NULL;

        config_find_array(state->config, "monitor.offsets", &offset_array);

        if (offset_array == NULL || offset_array->length == 0) {
                // Nothing to do here
                return;
        } else if (monitor_list->monitors->size > offset_array->length) {
                LOG_WARNING(natwm_logger,
                            "Encountered more monitors than items in "
                            "'monitor.offsets' array. Ignoring offsets");

                return;
        }

        size_t index = 0;

        LIST_FOR_EACH(monitor_list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;
                // By default we will have no offset
                struct box_sizes offsets = {
                        .top = 0,
                        .right = 0,
                        .bottom = 0,
                        .left = 0,
                };

                const struct config_value *offset_array_value = offset_array->values[index];

                if (offset_array_value->type != ARRAY) {
                        LOG_WARNING(natwm_logger, "Skipping invalid monitor offset value");
                        continue;
                }

                enum natwm_error err
                        = config_array_to_box_sizes(offset_array_value->data.array, &offsets);

                if (err != NO_ERROR) {
                        LOG_WARNING(natwm_logger, "Skipping invalid monitor offset value");
                        break;
                }

                monitor->offsets = offsets;

                ++index;
        }
}

static enum natwm_error monitors_from_randr(const struct natwm_state *state, struct list **result)
{
        struct list *monitor_list = list_create();

        if (monitor_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        struct randr_monitor **monitors = NULL;
        size_t monitor_length = 0;
        enum natwm_error err = randr_get_screens(state, &monitors, &monitor_length);

        if (err != NO_ERROR) {
                list_destroy(monitor_list);

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

                struct monitor *monitor
                        = monitor_create(randr_monitor->id, randr_monitor->rect, NULL);

                if (monitor == NULL) {
                        monitors_destroy(monitor_list);
                        list_destroy(monitor_list);

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
        struct list *monitor_list = list_create();

        if (monitor_list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        xcb_rectangle_t *rects = NULL;
        size_t monitor_length = 0;
        enum natwm_error err = xinerama_get_screens(state, &rects, &monitor_length);

        if (err != NO_ERROR) {
                list_destroy(monitor_list);

                return err;
        }

        for (size_t i = 0; i < monitor_length; ++i) {
                struct monitor *monitor = monitor_create((uint32_t)i, rects[i], NULL);

                if (monitor == NULL) {
                        monitors_destroy(monitor_list);
                        list_destroy(monitor_list);
                        free(rects);

                        return MEMORY_ALLOCATION_ERROR;
                }

                list_insert(monitor_list, monitor);
        }

        free(rects);

        *result = monitor_list;

        return NO_ERROR;
}

static enum natwm_error monitor_from_x(const struct natwm_state *state, struct list **result)
{
        struct list *monitor_list = list_create();

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
                list_destroy(monitor_list);

                return MEMORY_ALLOCATION_ERROR;
        }

        list_insert(monitor_list, monitor);

        *result = monitor_list;

        return NO_ERROR;
}

struct server_extension *server_extension_detect(xcb_connection_t *connection)
{
        struct server_extension *extension = malloc(sizeof(struct server_extension));

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
}

struct monitor_list *monitor_list_create(struct server_extension *extension, struct list *monitors)
{
        struct monitor_list *list = malloc(sizeof(struct monitor_list));

        if (list == NULL) {
                return NULL;
        }

        list->extension = extension;
        list->monitors = monitors;

        return list;
}

struct monitor *monitor_list_get_active_monitor(const struct monitor_list *monitor_list)
{
        LIST_FOR_EACH(monitor_list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;

                if (monitor->workspace != NULL && monitor->workspace->is_focused) {
                        return monitor;
                }
        }

        return NULL;
}

struct monitor *monitor_list_get_workspace_monitor(const struct monitor_list *monitor_list,
                                                   const struct workspace *workspace)
{
        if (workspace == NULL) {
                return NULL;
        }

        LIST_FOR_EACH(monitor_list->monitors, monitor_item)
        {
                struct monitor *monitor = (struct monitor *)monitor_item->data;

                if (monitor->workspace == NULL) {
                        continue;
                }

                if (monitor->workspace->index == workspace->index) {
                        return monitor;
                }
        }

        return NULL;
}

struct monitor *monitor_create(uint32_t id, xcb_rectangle_t rect, struct workspace *workspace)
{
        struct monitor *monitor = malloc(sizeof(struct monitor));

        if (monitor == NULL) {
                return NULL;
        }

        monitor->id = id;
        monitor->rect = rect;
        monitor->workspace = workspace;

        return monitor;
}

enum natwm_error monitor_setup(const struct natwm_state *state, struct monitor_list **result)
{
        struct server_extension *extension = server_extension_detect(state->xcb);

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

        struct monitor_list *monitor_list = monitor_list_create(extension, monitors);

        if (monitor_list == NULL) {
                free(extension);
                list_destroy(monitors);

                return MEMORY_ALLOCATION_ERROR;
        }

        monitor_list_set_offsets(state, monitor_list);

        // Initialize the desktop viewport
        ewmh_update_desktop_viewport(state, monitor_list);

        *result = monitor_list;

        return NO_ERROR;
}

xcb_rectangle_t monitor_clamp_client_rect(const struct monitor *monitor,
                                          xcb_rectangle_t client_rect)
{
        xcb_rectangle_t monitor_rect = monitor_get_offset_rect(monitor);

        int32_t x = client_rect.x;
        int32_t y = client_rect.y;
        int32_t width = client_rect.width;
        int32_t height = client_rect.height;
        int32_t total_client_width = client_rect.x + client_rect.width - monitor->offsets.left;
        int32_t total_client_height = client_rect.y + client_rect.height - monitor->offsets.top;

        if (total_client_width > monitor_rect.width) {
                int32_t overflow = total_client_width - monitor_rect.width;
                int32_t subtract_from_width = overflow - (client_rect.x - monitor->offsets.left);

                // If we don't have enough x offset to account for the overflow
                // we need to mutate the width of the client to fit onto the
                // monitor
                if (subtract_from_width > 0) {
                        x = monitor->offsets.left;
                        width = client_rect.width - subtract_from_width;
                } else {
                        x = client_rect.x - overflow;
                }
        } else if (client_rect.x < monitor->offsets.left) {
                x = monitor->offsets.left;
        }

        if (total_client_height > monitor_rect.height) {
                int32_t overflow = total_client_height - monitor_rect.height;
                int32_t subtract_from_height = overflow - (client_rect.y - monitor->offsets.top);

                if (subtract_from_height > 0) {
                        y = monitor->offsets.top;
                        height = client_rect.height - subtract_from_height;
                } else {
                        y = client_rect.y - overflow;
                }
        } else if (client_rect.y < monitor->offsets.top) {
                y = monitor->offsets.top;
        }

        xcb_rectangle_t new_rect = {
                .x = (int16_t)x,
                .y = (int16_t)y,
                .width = (uint16_t)width,
                .height = (uint16_t)height,
        };

        return new_rect;
}

xcb_rectangle_t monitor_get_offset_rect(const struct monitor *monitor)
{
        xcb_rectangle_t rect = {
                .x = 0,
                .y = 0,
                .width = 0,
                .height = 0,
        };

        if (monitor == NULL) {
                return rect;
        }

        struct box_sizes offsets = monitor->offsets;

        rect.x = (int16_t)(monitor->rect.x + offsets.left);
        rect.y = (int16_t)(monitor->rect.y + offsets.top);
        rect.width = (uint16_t)(monitor->rect.width - (offsets.left + offsets.right));
        rect.height = (uint16_t)(monitor->rect.height - (offsets.top + offsets.bottom));

        return rect;
}

void monitor_list_destroy(struct monitor_list *monitor_list)
{
        free(monitor_list->extension);
        monitors_destroy(monitor_list->monitors);
        list_destroy(monitor_list->monitors);
        free(monitor_list);
}

void monitor_destroy(struct monitor *monitor)
{
        free(monitor);
}
