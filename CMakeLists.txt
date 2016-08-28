cmake_minimum_required(VERSION 3.0)
project(appbase-cctv C)

set(CMAKE_C_FLAGS "-std=c11 -Wall")
set(CMAKE_C_FLAGS_DEBUG "-g")

# Library with common functions #
set(library-srcs appbase.c uvc.c frame.c utils.c config.c json-streamer.c cb.c)
add_library(appbase-common SHARED ${library-srcs})

target_compile_definitions(appbase-common PUBLIC "_GNU_SOURCE=1")
target_link_libraries(appbase-common "curl" "json-c" "modpbase64" "jpeg" "yajl" "SDL2_image" "pthread")

# Daemon #
set(daemon-srcs daemon-main.c)
add_executable(appbase-cctv-daemon ${daemon-srcs})

target_link_libraries(appbase-cctv-daemon appbase-common)

# Client #
set(client-srcs client-main.c sdl-window.c)
add_executable(appbase-cctv-client ${client-srcs})

target_link_libraries(appbase-cctv-client appbase-common "SDL2")
