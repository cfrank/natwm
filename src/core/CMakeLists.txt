add_library(core STATIC
    config.c
    config.h
    ewmh.c
    ewmh.h
    list.c
    list.h
    map.c
    map.h
    state.c
    state.h
)

target_link_libraries(core
    PRIVATE
        common
        xcb
        xcb-ewmh
)
