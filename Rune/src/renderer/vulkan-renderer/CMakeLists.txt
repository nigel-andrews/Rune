target_sources(Rune PRIVATE backend.cc)

set(BUILD_SHARED_LIBS True)

find_package(Vulkan REQUIRED)

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

target_link_libraries(Rune PUBLIC vulkan vk-bootstrap::vk-bootstrap)
