add_library(application OBJECT application.cc)

target_link_libraries(application PRIVATE compile_options vendor)
