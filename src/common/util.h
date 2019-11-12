// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int string_to_number(const char *number_string, intmax_t *dest);

// fs utils
ssize_t get_file_size(FILE *file);
bool path_exists(const char *path);
