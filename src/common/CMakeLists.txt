add_library(common STATIC
    constants.h
    hash.h
    hash.c
    logger.c
    logger.h
    util.c
    util.h
)

target_link_libraries(common
    PUBLIC clog
)
