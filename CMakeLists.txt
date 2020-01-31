cmake_minimum_required(VERSION 3.15)
project(raytracerproj C)

set(CMAKE_C_STANDARD 99)

add_executable(raytracerproj main.c ppm_file.c)