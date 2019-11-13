// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "error.h"

char *string_init(const char *string);
enum natwm_error string_append(char **destination, const char *append);
enum natwm_error string_append_char(char **destination, char append);
enum natwm_error string_find_char(const char *haystack, char needle,
                                  size_t *index);
enum natwm_error string_find_first_nonspace(const char *string,
                                            size_t *index_result);
enum natwm_error string_find_last_nonspace(const char *string,
                                           size_t *index_result);
enum natwm_error string_get_delimiter(const char *string, char delimiter,
                                      char **destination, size_t *length,
                                      bool consume);
enum natwm_error string_splice(const char *string, size_t start, size_t end,
                               char **destination, size_t *size);
enum natwm_error string_strip_surrounding_spaces(const char *string,
                                                 char **dest, size_t *length);
enum natwm_error string_to_number(const char *number_string, intmax_t *dest);
