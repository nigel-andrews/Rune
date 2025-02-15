#pragma once

#include <filesystem>
#include <vulkan/vulkan.hpp>

namespace fs = std::filesystem;

namespace Rune::Vulkan
{
    std::optional<vk::ShaderModule> load_shader(const fs::path& shader_path,
                                                vk::Device device);
} // namespace Rune::Vulkan
