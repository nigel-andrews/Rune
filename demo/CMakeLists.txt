project(demo)

add_executable(demo src/main.cpp)

set_target_properties(demo PROPERTIES CXX_STANDARD 23)

target_link_libraries(demo PRIVATE Rune)
