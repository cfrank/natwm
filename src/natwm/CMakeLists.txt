add_executable(natwm
    natwm.c
)

set_target_properties(natwm
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    C_STANDARD 99
    C_EXTENSIONS OFF
)

target_link_libraries(natwm
    PUBLIC core
    xcb
)
