// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#include <unistd.h>

#include "logger.h"
#include "util.h"

enum natwm_error config_array_to_box_sizes(const struct config_array *array,
                                           struct box_sizes *result)
{
        if (array == NULL || array->length != 4) {
                return INVALID_INPUT_ERROR;
        }

        struct box_sizes sizes = {
                .top = 0,
                .right = 0,
                .bottom = 0,
                .left = 0,
        };

        for (size_t i = 0; i < 4; ++i) {
                struct config_value *value = array->values[i];

                if (value == NULL || value->type != NUMBER) {
                        return INVALID_INPUT_ERROR;
                }

                assert(value->data.number <= UINT16_MAX);
        }

        sizes.top = (uint16_t)array->values[0]->data.number;
        sizes.right = (uint16_t)array->values[1]->data.number;
        sizes.bottom = (uint16_t)array->values[2]->data.number;
        sizes.left = (uint16_t)array->values[3]->data.number;

        *result = sizes;

        return NO_ERROR;
}

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
