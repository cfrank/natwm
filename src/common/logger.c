#include "logger.h"

struct logger *natwm_logger;

void initialize_logger(void)
{
        natwm_logger = create_logger("NATWM");

        if (DEBUG == 1) {
                set_logging_min_level(natwm_logger, LEVEL_TRACE);
        }
}
