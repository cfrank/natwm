// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <string.h>

#include <common/logger.h>
#include <common/string.h>

#include "value.h"

static struct config_array *config_array_create(size_t length)
{
        struct config_array *array = malloc(sizeof(struct config_array));

        if (array == NULL) {
                return NULL;
        }

        array->length = length;
        array->values = calloc(length, sizeof(struct config_value *));

        if (array->values == NULL) {
                free(array);

                return NULL;
        }

        return array;
}

static struct config_array *config_array_duplicate(const struct config_array *array)
{
        struct config_array *new_array = config_array_create(array->length);

        if (new_array == NULL) {
                return NULL;
        }

        for (size_t i = 0; i < new_array->length; ++i) {
                struct config_value *duplicated_value = config_value_duplicate(array->values[i]);

                if (duplicated_value == NULL) {
                        goto free_and_error;
                }

                new_array->values[i] = duplicated_value;
        }

        return new_array;

free_and_error:
        config_array_destroy(new_array);

        return NULL;
}

void config_array_destroy(struct config_array *array)
{
        for (size_t i = 0; i < array->length; ++i) {
                struct config_value *value_to_destroy = array->values[i];

                if (value_to_destroy != NULL) {
                        config_value_destroy(value_to_destroy);
                }
        }

        free(array->values);
        free(array);
}

struct config_value *config_value_create_array(size_t length)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->type = ARRAY;

        value->data.array = config_array_create(length);

        if (value->data.array == NULL) {
                free(value);

                return NULL;
        }

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

struct config_value *config_value_duplicate(const struct config_value *value)
{
        struct config_value *new_value = malloc(sizeof(struct config_value));

        if (new_value == NULL) {
                return NULL;
        }

        memcpy(new_value, value, sizeof(struct config_value));

        switch (value->type) {
        case ARRAY:
                new_value->data.array = config_array_duplicate(value->data.array);

                if (new_value->data.array == NULL) {
                        goto free_and_error;
                }

                break;
        case BOOLEAN:
                new_value->data.boolean = value->data.boolean;

                break;
        case STRING:
                new_value->data.string = string_init(value->data.string);

                if (new_value->data.string == NULL) {
                        goto free_and_error;
                }

                break;
        case NUMBER:
                new_value->data.number = value->data.number;

                break;
        default:
                goto free_and_error;
        }

        return new_value;

free_and_error:
        free(new_value);

        return NULL;
}

void config_value_destroy(struct config_value *value)
{
        if (value->type == STRING) {
                // For strings we need to free the data as well
                free(value->data.string);
        }

        if (value->type == ARRAY && value->data.array != NULL) {
                config_array_destroy(value->data.array);
        }

        free(value);
}
