add_library(core OBJECT memory/deletion_stack.cc)

set_target_properties(core PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF)

target_link_libraries(core PRIVATE compile_options)
