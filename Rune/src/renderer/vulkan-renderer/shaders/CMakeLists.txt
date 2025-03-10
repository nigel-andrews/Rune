compile_glsl(gradient comp)
compile_glsl(sky comp)
compile_glsl(triangle vert)
compile_glsl(triangle frag)

target_compile_definitions(
  vulkan-backend PRIVATE DEFAULT_SHADER_PATH="${CMAKE_CURRENT_BINARY_DIR}")
