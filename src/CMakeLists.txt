include_directories(.)

add_compile_options(
    -march=native
    -Wall
    -pedantic
    -Wextra
    -Wstrict-overflow
    -Wconversion
    -fno-strict-aliasing
    -D_POSIX_C_SOURCE=200809L
    "$<$<CONFIG:DEBUG>:-g>"
    "$<$<CONFIG:DEBUG>:-Werror>"
    "$<$<CONFIG:DEBUG>:-Wshadow>"
    "$<$<CONFIG:RELEASE>:-O3 -Os>"
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(common)

add_subdirectory(core)

add_subdirectory(natwm)

if(ENABLE_TESTING)
    add_subdirectory(test)
endif()
