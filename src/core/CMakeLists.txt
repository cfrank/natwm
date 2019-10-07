add_library(core STATIC
    config.c
    config.h
    list.c
    list.h
    map.c
    map.h
)

target_link_libraries(core
    PRIVATE common
)
