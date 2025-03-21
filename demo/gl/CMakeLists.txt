project(gl_demo)

add_executable(gl_demo src/main.cpp)

set_target_properties(gl_demo PROPERTIES CXX_STANDARD 23)

target_link_libraries(gl_demo PRIVATE Rune)
