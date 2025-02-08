# add_custom_command( OUTPUT vert.spv COMMAND glslc
# ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert -o vert.spv)
#
# add_custom_command( OUTPUT frag.spv COMMAND glslc
# ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag -o frag.spv)

add_custom_command(
  OUTPUT compute.spv
  COMMAND glslc ${CMAKE_CURRENT_SOURCE_DIR}/shader.comp -o compute.spv
  COMMENT "")

add_custom_target(
  shaders ALL
  DEPENDS compute.spv #[[ vert.spv frag.spv ]]
  COMMENT "Compiling compute shader to SPIR-V")
