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
    add_compile_definitions(IS_DEBUG_BUILD=1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(IS_DEBUG_BUILD=0)
else()
    message(STATUS "Defauling CMAKE_BUILD_TYPE to \"Debug\"")
    set(CMAKE_BUILD_TYPE "Debug")
    add_compile_definitions(IS_DEBUG_BUILD=1)
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

if(ENABLE_TESTING)
    include(CTest)
    include(AddNatwmTest)
    # Add a target for calling ctest with verbose output on failure
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    )
endif()

add_subdirectory(vendor)
add_subdirectory(src)
