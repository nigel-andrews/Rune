add_library(vendor INTERFACE)

find_package(SDL2 REQUIRED)

target_include_directories(vendor INTERFACE imgui ${SDL2_INCLUDE_DIRS})

file(GLOB vendor_SOURCES "imgui/*.cpp" "imgui/*.h"
     "imgui/backends/imgui_impl_sdl2.cpp" "imgui/misc/cpp/imgui_stdlib.*")

target_sources(vendor INTERFACE ${vendor_SOURCES})

target_link_libraries(vendor INTERFACE ${SDL2_LIBRARIES})
