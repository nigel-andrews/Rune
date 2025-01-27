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

FetchContent_Declare(
  vma
  GIT_REPOSITORY
    https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG v3.2.0)
FetchContent_MakeAvailable(vma)

add_library(Rune SHARED)

add_subdirectory(src)

target_include_directories(Rune PUBLIC src)

set_target_properties(Rune PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF)

target_link_libraries(Rune PUBLIC compile_options glfw vendor
                                  VulkanMemoryAllocator)
