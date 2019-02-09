add_subdirectory(natwm)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_subdirectory(test)
endif()
