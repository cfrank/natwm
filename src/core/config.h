// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

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
 * Represents a single config value
 */
struct config_value {
        enum config_data_types type;
        union {
                int64_t number;
                const char *string;
        } data;
};

/**
 * Handle key value pairs
 */
struct config_pair {
        const char *key;
        struct config_value *value;
};

/**
 * Holds all the configuration pairs
 */
struct config_list {
        uint32_t length;
        struct config_pair **pairs;
};

struct config_value *get_config_value(const struct config_list *list,
                                      const char *key);
void initialize_config(const char *path);
