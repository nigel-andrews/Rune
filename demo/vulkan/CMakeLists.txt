project(vulkan_demo)

add_executable(vulkan_demo src/main.cpp)

set_target_properties(vulkan_demo PROPERTIES CXX_STANDARD 23)

target_link_libraries(vulkan_demo PRIVATE Rune)
