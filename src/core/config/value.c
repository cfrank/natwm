// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <string.h>

#include <common/logger.h>
#include <common/string.h>

#include "value.h"

struct config_value *config_value_create_array(size_t length)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->type = ARRAY;

        struct config_array *array = malloc(sizeof(struct config_array));

        if (array == NULL) {
                free(value);

                return NULL;
        }

        array->length = length;
        array->values = malloc(sizeof(struct config_value *) * length);

        if (array->values == NULL) {
                free(value);
                free(array);

                return NULL;
        }

        value->data.array = array;

        return value;
}

struct config_value *config_value_create_boolean(bool boolean)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->type = BOOLEAN;
        value->data.boolean = boolean;

        return value;
}

struct config_value *config_value_create_number(intmax_t number)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->type = NUMBER;
        value->data.number = number;

        return value;
}

struct config_value *config_value_create_string(char *string)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->type = STRING;
        value->data.string = string;

        return value;
}

struct config_value *
config_value_duplicate(const struct config_value *config_value)
{
        struct config_value *new_value = malloc(sizeof(struct config_value));

        if (new_value == NULL) {
                return NULL;
        }

        memcpy(new_value, config_value, sizeof(struct config_value));

        if (config_value->type == STRING) {
                new_value->data.string = string_init(config_value->data.string);
        } else if (config_value->type == NUMBER) {
                new_value->data.number = config_value->data.number;
        }

        return new_value;
}

void config_value_destroy(struct config_value *value)
{
        if (value->type == STRING) {
                // For strings we need to free the data as well
                free(value->data.string);
        }

        if (value->type == ARRAY && value->data.array != NULL) {
                for (size_t i = 0; i < value->data.array->length; ++i) {
                        config_value_destroy(value->data.array->values[i]);
                }

                free(value->data.array->values);
                free(value->data.array);
        }

        free(value);
}
