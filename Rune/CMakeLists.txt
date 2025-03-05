set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(Rune SHARED)

add_subdirectory(src)

if(NOT FOUND_API)
  message(
    FATAL_ERROR
      "No supported graphics API was found (only Vulkan is supported currently)"
  )
endif()

target_include_directories(compile_options INTERFACE src)

set_target_properties(Rune PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF)

target_link_libraries(Rune PUBLIC compile_options vendor application platform
                                  core)
