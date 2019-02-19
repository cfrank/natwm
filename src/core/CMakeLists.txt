add_library(core STATIC
    config.c
    config.h
    list.c
    list.h
)

target_link_libraries(core
    PRIVATE common
)
