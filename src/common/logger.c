#include "logger.h"

void initialize_logger(void)
{
        if (DEBUG == 1) {
                set_logging_min_level(natwm_logger, LEVEL_TRACE);
        }
}
