add_library(common STATIC
    logger.c
    logger.h
    util.c
    util.h
)

target_link_libraries(common
    PUBLIC clog
)
