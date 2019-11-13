// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

enum natwm_error {
        MEMORY_ALLOCATION_ERROR = 1U << 0U, // Failed to allocate memory
        NOT_FOUND_ERROR = 1U << 1U, // Failed to find what was requested
        CAPACITY_ERROR = 1U << 2U, // Capacity exceeded
        INVALID_INPUT_ERROR = 1U << 3U, // Bad input received
        GENERIC_ERROR = 1U << 4U, // Catch-all error
        NO_ERROR = 1U << 5U, // No error
};

const char *error_to_string(enum natwm_error error);
