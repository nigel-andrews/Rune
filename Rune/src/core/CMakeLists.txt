add_library(core OBJECT memory/deletion_stack.cc)

target_link_libraries(core PRIVATE compile_options)
