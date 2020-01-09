// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/logger.h>
#include <core/config/config.h>

#include "theme.h"

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
                          hex_string);

                return INVALID_INPUT_ERROR;
        }

        *result = rgb;

        return NO_ERROR;
}

static enum natwm_error
color_value_from_config_value(const struct config_value *config_value,
                              struct color_value **result)
{
        if (config_value == NULL || config_value->type != STRING) {
                return INVALID_INPUT_ERROR;
        }

        struct color_value *value = NULL;
        enum natwm_error err
                = color_value_from_string(config_value->data.string, &value);

        if (err != NO_ERROR) {
                return err;
        }

        *result = value;

        return NO_ERROR;
}

struct border_theme *border_theme_create(void)
{
        struct border_theme *theme = malloc(sizeof(struct border_theme));

        if (theme == NULL) {
                return NULL;
        }

        theme->unfocused = 0;
        theme->focused = 0;
        theme->urgent = 0;
        theme->sticky = 0;

        return theme;
}

struct color_theme *color_theme_create(void)
{
        struct color_theme *theme = malloc(sizeof(struct color_theme));

        if (theme == NULL) {
                return NULL;
        }

        theme->unfocused = NULL;
        theme->focused = NULL;
        theme->urgent = NULL;
        theme->sticky = NULL;

        return theme;
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

enum natwm_error border_theme_from_config(const struct map *map,
                                          const char *key,
                                          struct border_theme **result)
{
        const struct config_array *config_value = NULL;

        if (config_find_array(map, key, &config_value) != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to find config item for '%s'",
                          key);

                return NOT_FOUND_ERROR;
        }

        struct border_theme *theme = border_theme_create();

        if (theme == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        theme->unfocused = DEFAULT_BORDER_WIDTH;
        theme->focused = DEFAULT_BORDER_WIDTH;
        theme->urgent = DEFAULT_BORDER_WIDTH;
        theme->sticky = DEFAULT_BORDER_WIDTH;

        if (config_value->length != 4) {
                *result = theme;

                return INVALID_INPUT_ERROR;
        }

        const struct config_value *unfocused_value = config_value->values[0];

        if (unfocused_value->type != NUMBER) {
                LOG_WARNING(
                        natwm_logger,
                        "Ignoring invalid unfocused config item inside '%s'",
                        key);
        } else {
                theme->unfocused = (uint16_t)unfocused_value->data.number;
        }

        const struct config_value *focused_value = config_value->values[1];

        if (focused_value->type != NUMBER) {
                LOG_WARNING(natwm_logger,
                            "Ignoring invalid focused config item inside '%s'",
                            key);
        } else {
                theme->focused = (uint16_t)focused_value->data.number;
        }

        const struct config_value *urgent_value = config_value->values[2];

        if (urgent_value->type != NUMBER) {
                LOG_WARNING(natwm_logger,
                            "Ignoring invalid urgent config item inside '%s'",
                            key);
        } else {
                theme->urgent = (uint16_t)urgent_value->data.number;
        }

        const struct config_value *sticky_value = config_value->values[2];

        if (sticky_value->type != NUMBER) {
                LOG_WARNING(natwm_logger,
                            "Ignoring invalid sticky config item inside '%s'",
                            key);
        } else {
                theme->sticky = (uint16_t)sticky_value->data.number;
        }

        *result = theme;

        return NO_ERROR;
}

enum natwm_error color_theme_from_config(const struct map *map, const char *key,
                                         struct color_theme **result)
{
        const struct config_array *config_value = NULL;

        if (config_find_array(map, key, &config_value) != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to find config item for '%s'",
                          key);

                return NOT_FOUND_ERROR;
        }

        // TODO: It might be better in the future to leave unset config values
        // as NULL and have a fallback. That way users could just override the
        // values they wanted to change.
        if (config_value->length != 4) {
                LOG_ERROR(natwm_logger,
                          "Invalid number of values for config item '%s', "
                          "Expected 4");

                return INVALID_INPUT_ERROR;
        }

        struct color_theme *theme = color_theme_create();

        if (theme == NULL) {
                LOG_ERROR(natwm_logger, "Failed while allocating theme value");

                return MEMORY_ALLOCATION_ERROR;
        }

        enum natwm_error err = GENERIC_ERROR;

        err = color_value_from_config_value(config_value->values[0],
                                            &theme->unfocused);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid unfocused color value found in '%s'",
                          key);

                goto invalid_color_value_error;
        }

        err = color_value_from_config_value(config_value->values[1],
                                            &theme->focused);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid focused color value found in '%s'",
                          key);

                goto invalid_color_value_error;
        }

        err = color_value_from_config_value(config_value->values[2],
                                            &theme->urgent);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid urgent color value found in '%s'",
                          key);

                goto invalid_color_value_error;
        }

        err = color_value_from_config_value(config_value->values[3],
                                            &theme->sticky);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid sticky color value found in '%s'",
                          key);

                goto invalid_color_value_error;
        }

        *result = theme;

        return NO_ERROR;

invalid_color_value_error:
        color_theme_destroy(theme);

        return INVALID_INPUT_ERROR;
}

bool color_value_has_changed(struct color_value *value,
                             const char *new_string_value)
{
        if (strcmp(value->string, new_string_value) == 0) {
                return true;
        }

        return false;
}

void border_theme_destroy(struct border_theme *theme)
{
        free(theme);
}

void color_value_destroy(struct color_value *value)
{
        free(value);
}

void color_theme_destroy(struct color_theme *theme)
{
        if (theme->unfocused != NULL) {
                color_value_destroy(theme->unfocused);
        }

        if (theme->focused != NULL) {
                color_value_destroy(theme->focused);
        }

        if (theme->urgent != NULL) {
                color_value_destroy(theme->urgent);
        }

        if (theme->sticky != NULL) {
                color_value_destroy(theme->sticky);
        }

        free(theme);
}
