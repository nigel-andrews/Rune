add_library(application STATIC application.cc)

target_link_libraries(application PRIVATE compile_options vendor)
