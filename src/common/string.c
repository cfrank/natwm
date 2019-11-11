// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "string.h"

/*
 * Duplicates a stack allocated string to a heap allocated string. This allows
 * for mutating/building upon a string with for instance the string_append
 * function
 */
ATTR_NONNULL char *string_init(const char *string)
{
        // Include null terminator
        size_t length = strlen(string) + 1;
        char *result = malloc(length);

        if (result == NULL) {
                return NULL;
        }

        memcpy(result, string, length);

        return result;
}
