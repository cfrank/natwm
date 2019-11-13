add_library(common STATIC
    constants.h
    error.c
    error.h
    hash.h
    hash.c
    logger.c
    logger.h
    string.c
    string.h
    util.c
    util.h
)

target_link_libraries(common
    PUBLIC clog
)
