// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stddef.h>

#include "error.h"

const char *error_to_string(enum natwm_error error)
{
        switch (error) {
        case MEMORY_ALLOCATION_ERROR:
                return "Failed to allocate memory";
        case NOT_FOUND_ERROR:
                return "Failed to find what was requested";
        case CAPACITY_ERROR:
                return "Capacity exceeded";
        case INVALID_INPUT_ERROR:
                return "Bad input received";
        case RESOLUTION_FAILURE:
                return "Failure during function resolution";
        case GENERIC_ERROR:
                return "Generic error";
        case NO_ERROR:
                return "No error";
        default:
                return "Invalid error";
        }

        return NULL;
}
