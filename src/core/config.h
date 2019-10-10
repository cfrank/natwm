// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>

#include "map.h"

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

struct config_value *config_find(const struct map *config_map, const char *key);
void config_destroy(struct map *config_map);
void config_value_destroy(struct config_value *value);

struct map *config_read_string(const char *config, size_t config_size);
struct map *config_initialize_path(const char *path);
