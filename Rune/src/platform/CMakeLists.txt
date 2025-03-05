add_library(platform OBJECT window.cc)

target_link_libraries(platform PRIVATE compile_options vendor)
