// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>

#include <common/error.h>
#include <common/map.h>

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

struct map *config_read_string(const char *config, size_t size);
struct map *config_initialize_path(const char *path);

struct config_value *config_find(const struct map *config_map, const char *key);
enum natwm_error config_find_number(const struct map *config_map,
                                    const char *key, intmax_t *result);
intmax_t config_find_number_fallback(const struct map *config_map,
                                     const char *key, intmax_t fallback);
const char *config_find_string_fallback(const struct map *config_map,
                                        const char *key, const char *fallback);
const char *config_find_string(const struct map *config_map, const char *key);
const char *config_find_string_fallback(const struct map *config_map,
                                        const char *key, const char *fallback);

void config_destroy(struct map *config_map);
void config_value_destroy(struct config_value *value);
