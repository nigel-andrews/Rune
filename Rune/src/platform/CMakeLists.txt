add_library(platform OBJECT window.cc)

set_target_properties(platform PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF)

target_link_libraries(platform PRIVATE compile_options vendor)
