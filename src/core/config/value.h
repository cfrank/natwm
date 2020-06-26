// Copyright 2020 Chris Frank
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
        ARRAY,
        BOOLEAN,
        NUMBER,
        STRING,
};

struct config_value;

/**
 * An array of config values
 */
struct config_array {
        size_t length;
        struct config_value **values;
};

/**
 * A config value
 */
struct config_value {
        enum data_types type;
        union {
                bool boolean;
                intmax_t number;
                // Heap allocated - must be free'd
                char *string;
                struct config_array *array;
        } data;
};

void config_array_destroy(struct config_array *array);

struct config_value *config_value_create_array(size_t length);
struct config_value *config_value_create_boolean(bool boolean);
struct config_value *config_value_create_number(intmax_t number);
struct config_value *config_value_create_string(char *string);
struct config_value *config_value_duplicate(const struct config_value *value);

void config_value_destroy(struct config_value *value);
