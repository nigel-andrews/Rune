target_sources(Rune PRIVATE renderer.cc)

# FIXME: Handle multiple renderers
add_subdirectory(vulkan-renderer)
