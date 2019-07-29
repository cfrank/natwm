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
        char *key;
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
        size_t size;
        struct config_value **values;
};

extern struct config_list *config;

struct config_value *get_config_value(const struct config_list *list,
                                      const char *key);
struct config_value *config_list_find(struct config_list *list,
                                      const char *key);
void destroy_config_list(struct config_list *list);
void destroy_config_value(struct config_value *value);

struct config_list *initialize_config_string(const char *config,
                                             size_t config_size);
struct config_list *initialize_config_path(const char *path);
