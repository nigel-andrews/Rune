add_library(vulkan-backend OBJECT backend.cc)

set(BUILD_SHARED_LIBS True)

find_package(Vulkan)

if(Vulkan_FOUND)
  message("-- Found Vulkan graphics API !")
  set(FOUND_API
      True
      CACHE BOOL "")

  FetchContent_Declare(
    vulkan-hpp
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Hpp.git
    GIT_TAG v1.4.303
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  FetchContent_MakeAvailable(vulkan-hpp)

  FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG v1.3.302)
  FetchContent_MakeAvailable(fetch_vk_bootstrap)

  FetchContent_Declare(
    vma
    GIT_REPOSITORY
      https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.2.0)
  FetchContent_MakeAvailable(vma)

  add_subdirectory(utils)

  # NOTE: CMake format changes this but CMake Lint isn't happy when running the
  # precommit.
  set_target_properties(vulkan-backend PROPERTIES CXX_STANDARD 23)
  set_target_properties(vulkan-backend PROPERTIES CXX_EXTENSIONS OFF)

  target_link_libraries(
    vulkan-backend
    PUBLIC compile_options
           core
           glfw
           vendor
           vulkan
           vk-bootstrap::vk-bootstrap
           VulkanMemoryAllocator)

  target_link_libraries(Rune PRIVATE vulkan-backend)

  add_subdirectory(shaders)
endif()
