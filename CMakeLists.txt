cmake_minimum_required(VERSION 3.0)
project(appbase-cctv C)

set (srcs main.c appbase.c uvc.c utils.c)
add_executable (appbase-cctv-daemon ${srcs})

add_compile_options("-Wall")
target_compile_definitions(appbase-cctv-daemon PUBLIC "_GNU_SOURCE=1")
target_link_libraries(appbase-cctv-daemon "curl" "json-c")
