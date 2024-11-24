cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Rune)

include(FetchContent)

add_library(compile_options INTERFACE)

target_compile_options(
  compile_options
  INTERFACE -Wall
            -Wextra
            -std=c++23
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-Werror>
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-fno-inline>
            $<$<CONFIG:Debug>:-g3>
            $<$<CONFIG:Debug>:-fsanitize=address>)

target_link_options(compile_options INTERFACE
                    $<$<CONFIG:Debug>:-fsanitize=address>)

target_compile_definitions(compile_options INTERFACE $<$<CONFIG:Release>:NDEBUG>
                                                     $<$<CONFIG:Debug>:DEBUG>)

add_executable(Rune src/main.cpp)

set_target_properties(Rune PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF)

target_link_libraries(Rune PRIVATE compile_options)
