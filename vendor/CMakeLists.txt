add_library(vendor STATIC)

set_property(TARGET vendor PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(vendor PUBLIC imgui)

file(
  GLOB
  vendor_SOURCES
  "imgui/*.cpp"
  "imgui/*.h"
  "imgui/backends/imgui_impl_glfw.cpp"
  "imgui/backends/imgui_impl_vulkan.cpp"
  "imgui/misc/cpp/imgui_stdlib.*")

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG refs/tags/1.0.1)
FetchContent_MakeAvailable(glm)

set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_INSTALL
    OFF
    CACHE BOOL "" FORCE)

# FIXME: Use 3.4
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9)
FetchContent_MakeAvailable(glfw)

target_sources(vendor PRIVATE ${vendor_SOURCES})

target_compile_options(vendor PRIVATE $<$<CONFIG:Release>:-O3>
                                      $<$<CONFIG:Debug>:-O0>)

target_link_libraries(vendor PUBLIC glm glfw)
