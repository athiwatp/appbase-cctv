cmake_minimum_required(VERSION 3.0)
project(appbase-cctv C)

set (srcs main.c appbase.c uvc.c utils.c)
add_executable (appbase-cctv-daemon ${srcs})

set(CMAKE_C_FLAGS "-std=c99 -Wall")
set(CMAKE_C_FLAGS_DEBUG "-g")
target_compile_definitions(appbase-cctv-daemon PUBLIC "_GNU_SOURCE=1")
target_link_libraries(appbase-cctv-daemon "curl" "json-c" "modpbase64")
