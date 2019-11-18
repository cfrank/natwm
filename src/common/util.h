// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "error.h"

// fs utils
enum natwm_error get_file_size(FILE *file, size_t *size);
bool path_exists(const char *path);

// time utils
void milisecond_sleep(uint32_t miliseconds);
