add_library(common STATIC
    constants.h
    error.c
    error.h
    hash.h
    hash.c
    list.c
    list.h
    logger.c
    logger.h
    map.c
    map.h
    stack.c
    stack.h
    string.c
    string.h
    theme.c
    theme.h
    tree.c
    tree.h
    types.h
    util.c
    util.h
)

target_link_libraries(common
    PUBLIC clog
)
