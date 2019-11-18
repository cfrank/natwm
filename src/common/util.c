// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#include <unistd.h>

#include "util.h"

/**
 * Get size of a file
 *
 * If the size of the file is able to be found it will be placed in the
 * provided pointer and NO_ERROR will be returned
 *
 * Otherwise GENERIC_ERROR will be returned and the errno will provide more
 * context
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

/**
 * Sleep for a spcified number of miliseconds
 */
void milisecond_sleep(uint32_t miliseconds)
{
        struct timeval tv = {
                0, // Seconds
                (suseconds_t)(miliseconds * 1000L), // Microseconds
        };

        int ret = 0;

        do {
                ret = select(1, NULL, NULL, NULL, &tv);
        } while ((ret == -1) && (errno == EINTR));
}
