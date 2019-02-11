cmake_minimum_required(VERSION 3.4)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(natwm
    VERSION 0.0.1
    DESCRIPTION "Not another tiling window manager"
    LANGUAGES C
)

set(PACKAGE_VERSION_STRING v${PROJECT_VERSION})
set(PROJECT_STRING "${PROJECT_NAME} ${PACKAGE_VERSION_STRING}")

add_compile_definitions(NATWM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(NATWM_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(NATWM_VERSION_PATCH=${PROJECT_VERSION_PATCH})
add_compile_definitions(NATWM_VERSION_STRING="${PROJECT_STRING}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG=1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(DEBUG=0)
else()
    message(STATUS "No CMAKE_BUILD_TYPE specified - Defaulting to \"Debug\"")
    set(CMAKE_BUILD_TYPE "Debug")
    add_compile_definitions(DEBUG=1)
endif()

option(ENABLE_TESTING "Enable automated testing" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_SRCS_DIRECTORY ${PROJECT_SOURCE_DIR}/src)

# Clang Format target

set(CLANG_FORMAT_POSTFIX "-7.0")
find_program(CLANG_FORMAT
    NAMES clang-format${CLANG_FORMAT_POSTFIX}
          clang-format
)

if(CLANG_FORMAT)
    add_custom_target(clang-format
        COMMAND find ${CMAKE_SRCS_DIRECTORY} -iname *.h -o -iname *.c | xargs ${CLANG_FORMAT} -i
        COMMENT "Running clang-format against *.{c,h} inside ${CMAKE_SRCS_DIRECTORY}"
    )
else()
    message(WARNING "Unable to find clang-format! Target was not created")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compilation flags

string(APPEND CMAKE_C_FLAGS_DEBUG " -g3"
    " -O")

string(APPEND CMAKE_C_FLAGS_RELEASE " -O2"
    " -march=native"
    " -mtune=native"
    " -ftree-vectorize")

string(APPEND CMAKE_C_FLAGS " -Wall"
    " -D_FORTIFY_SOURCE=2"
    " -Wextra -Wcast-align -Wcast-qual -Wpointer-arith"
    " -Waggregate-return -Wunreachable-code -Wfloat-equal"
    " -Wformat=2 -Wredundant-decls -Wundef"
    " -Wdisabled-optimization -Wshadow -Wmissing-braces"
    " -Wstrict-aliasing=2 -Wstrict-overflow=5 -Wconversion"
    " -Wno-unused-parameter"
    " -pedantic")

if(ENABLE_TESTING)
    include(CTest)
    include(AddNatwmTest)
endif()

add_subdirectory(vendor)
add_subdirectory(src)
