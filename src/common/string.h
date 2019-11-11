// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stddef.h>

#include "error.h"

char *string_init(const char *string);
enum natwm_error string_append(char **destination, const char *append);
