cmake_minimum_required(VERSION 3.15)
project(scon VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(scon scon.c)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE SCON_GIT_HASH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT SCON_GIT_HASH)
    set(SCON_GIT_HASH "unknown")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/scon_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/scon_version.h"
)

target_include_directories(scon PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS scon RUNTIME DESTINATION bin)

target_link_libraries(scon m)