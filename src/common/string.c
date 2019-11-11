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

/*
 * Takes a pointer to a heap allocated string and appends another string to the
 * end of it, making sure to add the null terminator to the end. The resulting
 * string is reallocated in place of the first argument, so it must be freed
 */
enum natwm_error string_append(char **destination, const char *append)
{
        size_t destination_size = strlen(*destination);
        size_t append_size = strlen(append);
        size_t result_size = destination_size + append_size;
        char *tmp = realloc(*destination, result_size + 1);

        if (tmp == NULL) {
                free(*destination);

                return MEMORY_ALLOCATION_ERROR;
        }

        *destination = tmp;

        strncpy(*destination + destination_size, append, append_size);

        (*destination)[result_size] = '\0';

        return NO_ERROR;
}

/*
 * Takes a pointer to a heap allocated string and appends a single char to the
 * end of it, making sure to add a null terminator to the end. The resulting
 * string is reallocated in place of the first argument, so it must be freed
 */
enum natwm_error string_append_char(char **destination, char append)
{
        size_t destination_size = strlen(*destination);
        char *tmp = realloc(*destination, destination_size + 2);

        if (tmp == NULL) {
                free(*destination);

                return MEMORY_ALLOCATION_ERROR;
        }

        *destination = tmp;

        (*destination)[destination_size] = append;
        (*destination)[destination_size + 1] = '\0';

        return NO_ERROR;
}
