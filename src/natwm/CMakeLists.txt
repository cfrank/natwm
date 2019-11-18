add_executable(natwm
    natwm.c
)

set_target_properties(natwm
    PROPERTIES
    VERSION ${PROJECT_VERSION}
)

target_link_libraries(natwm
    PRIVATE
        common
        core
        pthread
        xcb
)
