add_library(core STATIC memory/deletion_stack.cc)

target_link_libraries(core PRIVATE compile_options)
