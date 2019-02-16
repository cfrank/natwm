include_directories(.)

add_subdirectory(common)

add_subdirectory(core)

add_subdirectory(natwm)

if(ENABLE_TESTING)
    add_subdirectory(test)
endif()
