target_compile_definitions(
  vulkan-backend PRIVATE DEFAULT_SHADER_PATH="${CMAKE_CURRENT_BINARY_DIR}")

compile_glsl(default vert)
compile_glsl(default frag)
