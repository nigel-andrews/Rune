add_library(vendor STATIC)

target_include_directories(vendor PUBLIC imgui)

file(
  GLOB
  vendor_SOURCES
  "imgui/*.cpp"
  "imgui/*.h"
  "imgui/backends/imgui_impl_glfw.cpp"
  "imgui/backends/imgui_impl_vulkan.cpp"
  "imgui/misc/cpp/imgui_stdlib.*")

target_sources(vendor PRIVATE ${vendor_SOURCES})

target_compile_options(vendor PRIVATE -fPIC $<$<CONFIG:Release>:-O3>
                                      $<$<CONFIG:Debug>:-O0>)

target_link_libraries(vendor PRIVATE glfw)
