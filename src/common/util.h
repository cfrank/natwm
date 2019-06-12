// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char *alloc_string(const char *string);
int string_append(char **destination, const char *append);
int string_append_char(char **destination, char append);
ssize_t string_find_char(const char *haystack, char needle);
ssize_t string_find_first_nonspace(const char *string);
ssize_t string_find_last_nonspace(const char *string);
ssize_t string_get_delimiter(const char *string, char delimiter,
                             char **destination, bool consume);
ssize_t string_splice(const char *string, char **dest, ssize_t start,
                      ssize_t end);
ssize_t string_strip_surrounding_spaces(const char *string, char **dest);

// fs utils
ssize_t get_file_size(FILE *file);
bool path_exists(const char *path);
