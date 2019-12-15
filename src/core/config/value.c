// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <string.h>

#include <common/string.h>

#include "value.h"

struct config_value *config_value_create_number(char *key, intmax_t number)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->key = key;
        value->type = NUMBER;
        value->data.number = number;

        return value;
}

struct config_value *config_value_create_string(char *key, char *string)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->key = key;
        value->type = STRING;
        value->data.string = string;

        return value;
}

struct config_value *config_value_create_boolean(char *key, bool boolean)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->key = key;
        value->type = BOOLEAN;
        value->data.boolean = boolean;

        return value;
}

struct config_value *
config_value_duplicate(const struct config_value *config_value, char *new_key)
{
        struct config_value *new_value = malloc(sizeof(struct config_value));

        if (new_value == NULL) {
                return NULL;
        }

        memcpy(new_value, config_value, sizeof(struct config_value));

        new_value->key = new_key;

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

        free(value->key);
        free(value);
}
