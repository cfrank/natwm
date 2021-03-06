// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <core/config/config.h>

#include "error.h"
#include "types.h"

// Config utilities
enum natwm_error config_array_to_box_sizes(const struct config_array *array,
                                           struct box_sizes *result);

// fs utils
enum natwm_error get_file_size(FILE *file, size_t *size);
bool path_exists(const char *path);

// time utils
void millisecond_sleep(uint32_t miliseconds);
