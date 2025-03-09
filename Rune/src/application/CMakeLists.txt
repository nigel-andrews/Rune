add_library(application OBJECT application.cc)

set_target_properties(application PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF)

target_link_libraries(application PRIVATE compile_options vendor)
