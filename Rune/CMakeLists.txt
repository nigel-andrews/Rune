add_library(Rune SHARED)

add_subdirectory(src)

target_include_directories(Rune PUBLIC src)

target_link_libraries(Rune PRIVATE compile_options)
