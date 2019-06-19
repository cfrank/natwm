// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>

/**
 * The available data types for natwm configuration files
 *
 * TODO: Support for more data types will be added as needed
 */
enum config_data_types {
        NUMBER,
        STRING,
};

/**
 * Handle key value pairs
 */
struct config_value {
        const char *key;
        enum config_data_types type;
        union {
                intmax_t number;
                // Heap allocated - must be free'd
                char *string;
        } data;
};

/**
 * Holds all the configuration values
 */
struct config_list {
        size_t length;
        struct config_value **values;
};

void destroy_config_list(struct config_list *list);
void destroy_config_value(struct config_value *value);
struct config_value *get_config_value(const struct config_list *list,
                                      const char *key);
int initialize_config(const char *path);
