add_library(gl-backend OBJECT backend.cc)

find_package(OpenGL REQUIRED)

target_link_libraries(gl-backend PUBLIC compile_options core glfw vendor)

set_target_properties(gl-backend PROPERTIES CXX_STANDARD 23)
set_target_properties(gl-backend PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(Rune PRIVATE gl-backend)

add_subdirectory(shaders)
