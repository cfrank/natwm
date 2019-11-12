// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"
#include "util.h"

/**
 * Takes a string which correspondes to a base 10 number as well as a pointer
 * to intmax_t. If the number can be transformed successfully the destination
 * will point to the number and 0 is returned.
 *
 * If there is an error then -1 is returned and the destination is unchanged
 */
int string_to_number(const char *number_string, intmax_t *dest)
{
        char *endptr = NULL;

        // Reset errno to 0 so we can test for ERANGE
        errno = 0;

        intmax_t result = strtoimax(number_string, &endptr, 10);

        if (errno == ERANGE || endptr == number_string || *endptr != '\0') {
                return -1;
        }

        if (errno != 0) {
                LOG_CRITICAL_LONG(natwm_logger,
                                  "Unhandlable input recieved - Exiting...");

                exit(EXIT_FAILURE);
        }

        *dest = result;

        return 0;
}

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
