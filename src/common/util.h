// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

char *alloc_string(const char *string);
int string_append(char **destination, const char *append);
int string_append_char(char **destination, char append);