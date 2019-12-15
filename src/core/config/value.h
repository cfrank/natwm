// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * The available data types for natwm configuration files
 *
 * TODO: Support for more data types will be added as needed
 */
enum data_types {
        BOOLEAN,
        NUMBER_ARRAY,
        NUMBER,
        STRING_ARRAY,
        STRING,
};

/**
 * Handle key value pairs
 */
struct config_value {
        char *key;
        enum data_types type;
        union {
                bool boolean;
                struct list *list;
                intmax_t number;
                // Heap allocated - must be free'd
                char *string;
        } data;
};

struct config_value *config_value_create_number(char *key, intmax_t number);
struct config_value *config_value_create_string(char *key, char *string);
struct config_value *
config_value_duplicate(const struct config_value *config_value, char *new_key);
struct config_value *config_value_create_boolean(char *key, bool boolean);

void config_value_destroy(struct config_value *value);
