add_library(core
    list.c
    list.h
)

target_include_directories(core
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
