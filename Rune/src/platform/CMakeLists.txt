add_library(platform STATIC window.cc)

target_link_libraries(platform PRIVATE compile_options vendor)
