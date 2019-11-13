// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <unistd.h>

#include "util.h"

/**
 * Get size of a file
 *
 * return -1 if we can't find the size
 */
enum natwm_error get_file_size(FILE *file, size_t *size_result)
{
        fseek(file, 0L, SEEK_END);

        int64_t size = ftell(file);

        if (size < 0) {
                // Specifics can be found from errno
                return GENERIC_ERROR;
        }

        fseek(file, 0L, SEEK_SET);

        *size_result = (size_t)size;

        return NO_ERROR;
}

/*
 * Returns whether or not a path exists
 */
bool path_exists(const char *path)
{
        if (access(path, F_OK) != 0) {
                return false;
        }

        return true;
}
