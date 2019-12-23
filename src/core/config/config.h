// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>

#include <common/error.h>
#include <common/list.h>
#include <common/map.h>

#include "value.h"

struct map *config_read_string(const char *config, size_t size);
struct map *config_initialize_path(const char *path);

struct config_value *config_find(const struct map *config_map, const char *key);
enum natwm_error config_find_array(const struct map *config_map,
                                   const char *key,
                                   const struct config_array **result);
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
