add_library(Rune SHARED)

add_subdirectory(src)

target_include_directories(Rune PUBLIC include)

target_link_libraries(Rune PRIVATE compile_options)
