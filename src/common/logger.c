// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "logger.h"

struct logger *natwm_logger;

void initialize_logger(bool verbose)
{
        natwm_logger = create_logger("NATWM");

        if (IS_DEBUG_BUILD == 1 || verbose) {
                set_logging_min_level(natwm_logger, LEVEL_TRACE);
        }
}
