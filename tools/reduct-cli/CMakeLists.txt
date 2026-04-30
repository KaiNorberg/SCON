cmake_minimum_required(VERSION 3.15)
project(reduct VERSION 1.0.4 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
add_executable(reduct reduct.c)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE REDUCT_GIT_HASH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT REDUCT_GIT_HASH)
    set(REDUCT_GIT_HASH "unknown")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/reduct_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/reduct_version.h"
)

target_include_directories(reduct PRIVATE 
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    "${CMAKE_CURRENT_SOURCE_DIR}/../../reduct"
)

option(REDUCT_USE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(REDUCT_USE_SANITIZERS)
    if(NOT WIN32)
        target_compile_options(reduct PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(reduct PRIVATE -fsanitize=address,undefined)
    endif()
endif()

option(REDUCT_USE_FUZZER "Enable libFuzzer build" OFF)
if(REDUCT_USE_FUZZER)
    if(WIN32 OR APPLE)
        message(FATAL_ERROR "libFuzzer is only supported on Linux")
    endif()

    add_executable(fuzz fuzz.c)

    target_include_directories(fuzz PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        "${CMAKE_CURRENT_SOURCE_DIR}/../../reduct"
    )

    target_compile_options(fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined
    )
    if(NOT WIN32)
        target_link_libraries(fuzz m)
    endif()
endif()

if(NOT WIN32)
    target_compile_options(reduct PRIVATE -Wall -Wextra -Werror -Wno-uninitialized)
endif()

install(TARGETS reduct 
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ../../reduct/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/reduct
    FILES_MATCHING PATTERN "*.h"
)

if(NOT WIN32)
    target_link_libraries(reduct m)
endif()