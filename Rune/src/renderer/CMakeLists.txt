target_sources(Rune PRIVATE renderer.cc gui.cc)

# FIXME: Have the possibility to pass folder as argument
function(compile_glsl shader_name ext)
  if(NOT
     (ext STREQUAL "comp"
      OR ext STREQUAL "vert"
      OR ext STREQUAL "frag"
      OR ext STREQUAL "glsl"))
    message(
      FATAL_ERROR
        "Requested file extension `${ext}` was not an acceptable
    glsl file extension for shader `${shader_name}`. As of now only comp, vert,
    frag, and glsl are supported")
  endif()

  set(shader_file "${shader_name}.${ext}")
  set(shader_output "${shader_name}.${ext}.spv")

  add_custom_command(
    OUTPUT ${shader_output}
    COMMAND glslc ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file} -o ${shader_output}
    COMMENT "Compiling ${shader_file}"
    MAIN_DEPENDENCY "${shader_file}")

  add_custom_target(
    "${shader_name}_${ext}_spv_target" ALL
    DEPENDS ${shader_output}
    COMMENT "")
endfunction()

# FIXME: Handle multiple renderers
add_subdirectory(vulkan-renderer)
