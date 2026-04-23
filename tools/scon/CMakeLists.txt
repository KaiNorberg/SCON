cmake_minimum_required(VERSION 3.15)
project(scon VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(scon scon.c)

install(TARGETS scon RUNTIME DESTINATION bin)

target_link_libraries(scon m)