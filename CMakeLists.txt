cmake_minimum_required(VERSION 3.0)
project(appbase-cctv C)

set(CMAKE_C_FLAGS "-std=c11 -Wall")
set(CMAKE_C_FLAGS_DEBUG "-g")

# Library with common functions #
set(library-srcs appbase.c uvc.c frame.c utils.c json-streamer.c cb.c)
add_library(appbase-common SHARED ${library-srcs})

set(libraries "json-c" "modpbase64" "jpeg" "yajl" "SDL2_image" "pthread")
if(NOT DEFINED HTTP_CLIENT OR HTTP_CLIENT STREQUAL "curl")
  message(STATUS "Using HTTP client: libcurl")
  set(libraries "${libraries};curl")
  add_definitions(-DUSING_LIBCURL)
elseif(HTTP_CLIENT STREQUAL "wget")
  message(STATUS "Using HTTP client: libwget")
  set(libraries "${libraries};wget")
  add_definitions(-DUSING_LIBWGET)
endif()

target_compile_definitions(appbase-common PUBLIC "_GNU_SOURCE=1")
target_link_libraries(appbase-common ${libraries})

# Daemon #
set(daemon-srcs daemon-main.c)
add_executable(appbase-cctv-daemon ${daemon-srcs})

target_link_libraries(appbase-cctv-daemon appbase-common)

# Client #
set(client-srcs client-main.c sdl-window.c)
add_executable(appbase-cctv-client ${client-srcs})

target_link_libraries(appbase-cctv-client appbase-common "SDL2")
