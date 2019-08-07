add_library(core STATIC
    dict.c
    dict.h
    config.c
    config.h
    list.c
    list.h
)

target_link_libraries(core
    PRIVATE common
)
