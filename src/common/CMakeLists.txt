add_library(common STATIC
    logger.c
    logger.h
)

target_link_libraries(common
    PUBLIC clog
)
