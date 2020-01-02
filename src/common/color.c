// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <common/logger.h>
#include <core/config/config.h>

#include "color.h"

static enum natwm_error string_to_rgb(const char *hex_string, uint32_t *result)
{
        if (hex_string[0] != '#') {
                LOG_ERROR(natwm_logger,
                          "Missing '#' in color value '%s'",
                          hex_string);

                return INVALID_INPUT_ERROR;
        }

        if (strlen(hex_string) != 7) {
                LOG_ERROR(natwm_logger,
                          "Found a color value with an invalid length");

                return INVALID_INPUT_ERROR;
        }

        char *endptr = NULL;
        // Have to ignore the '#'
        uint32_t rgb = (uint32_t)strtoul(hex_string + 1, &endptr, 16);

        if (endptr == NULL) {
                LOG_ERROR(natwm_logger,
                          "Found an invalid color value: '%s'",
                          hex_string + 1);

                return INVALID_INPUT_ERROR;
        }

        *result = rgb | 0xff000000;

        return NO_ERROR;
}

enum natwm_error color_value_from_string(const char *string,
                                         struct color_value **result)
{
        struct color_value *value = malloc(sizeof(struct color_value));

        if (value == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        value->string = string;
        value->color_value = 0;

        if (string_to_rgb(string, &value->color_value) != NO_ERROR) {
                free(value);

                return INVALID_INPUT_ERROR;
        }

        *result = value;

        return NO_ERROR;
}

enum natwm_error color_value_from_config(const struct map *map, const char *key,
                                         struct color_value **result)
{
        const char *string = config_find_string(map, key);

        if (string == NULL) {
                LOG_ERROR(natwm_logger, "Missing config item for '%s'", key);

                return NOT_FOUND_ERROR;
        }

        struct color_value *value = NULL;
        enum natwm_error err = color_value_from_string(string, &value);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to retrieve color value from '%s'",
                          key);

                return err;
        }

        *result = value;

        return NO_ERROR;
}

bool color_value_has_changed(struct color_value *value,
                             const char *new_string_value)
{
        if (strcmp(value->string, new_string_value) == 0) {
                return true;
        }

        return false;
}

void color_value_destroy(struct color_value *value)
{
        free(value);
}
