// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

enum natwm_error {
        MEMORY_ALLOCATION_ERROR = 1 << 0, // Failed to allocate memory
        NOT_FOUND_ERROR = 1 << 1, // Failed to find what was requested
        CAPACITY_ERROR = 1 << 2, // Capacity exceeded
        INVALID_INPUT_ERROR = 1 << 3, // Bad input received
        GENERIC_ERROR = 1 << 4, // Catch-all error
        NO_ERROR = 1 << 5, // No error
};

const char *error_to_string(enum natwm_error error);
