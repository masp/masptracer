cmake_minimum_required(VERSION 2.8)
project(raytracerproj C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(libcork)
add_executable(masptracer main.c ppm_file.c)