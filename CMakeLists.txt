cmake_minimum_required(VERSION 2.8)
project(raytracerproj C)

set(CMAKE_C_STANDARD 99)

add_library(tracer ppm_file.c scene.c camera.c vec.c scene_config.c)
add_executable(masptracer main.c)
target_link_libraries(masptracer tracer)

option(BUILD_TESTING "" OFF)
include(CTest)
if (BUILD_TESTING)
    add_executable(scene_test scene_test.c)
    target_link_libraries(scene_test cunit tracer)
endif()