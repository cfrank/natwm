add_library(core STATIC
    config.c
    config.h
    event.c
    event.h
    ewmh.c
    ewmh.h
    list.c
    list.h
    map.c
    map.h
    screen.c
    screen.h
    state.c
    state.h
)

target_link_libraries(core
    PRIVATE
        common
        pthread
        xcb
        xcb-ewmh
)
