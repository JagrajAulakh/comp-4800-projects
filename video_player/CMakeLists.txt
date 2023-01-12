cmake_minimum_required(VERSION 3.19)
project(video_player C)

find_package(PkgConfig)

pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(CAIRO REQUIRED cairo)

include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})

set(CMAKE_C_STANDARD 99)

add_executable(main main.c)
target_link_libraries(main ${GTK_LIBRARIES} ${CAIRO_LIBRARIES} )
