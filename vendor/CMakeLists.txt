# All external libs

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add unity if debug build
    add_library(unity STATIC Unity/src/unity.c)
    target_include_directories(unity PUBLIC Unity/src)
endif()
