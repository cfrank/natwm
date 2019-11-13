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
ssize_t get_file_size(FILE *file)
{
        fseek(file, 0L, SEEK_END);

        ssize_t size = ftell(file);

        if (size < 0) {
                return -1;
        }

        fseek(file, 0L, SEEK_SET);

        return size;
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
