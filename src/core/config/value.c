// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

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

void config_value_destroy(struct config_value *value)
{
        if (value->type == STRING) {
                // For strings we need to free the data as well
                free(value->data.string);
        }

        free(value->key);
        free(value);
}
