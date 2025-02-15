#include "shaders.hh"

#include <fstream>

#include "core/logger.hh"
#include "utils/types.hh"

namespace Rune::Vulkan
{
    std::optional<vk::ShaderModule> load_shader(const fs::path& shader_path,
                                                vk::Device device)
    {
        std::ifstream shader_file(shader_path,
                                  std::ios::ate | std::ios::binary);

        if (!shader_file.is_open())
        {
            Logger::log(Logger::WARN, "Failed to open file at ", shader_path);
            return std::nullopt;
        }

        u64 code_size = shader_file.tellg();

        std::vector<u32> buffer(code_size / sizeof(u32));

        shader_file.seekg(0);
        shader_file.read((char*)buffer.data(), code_size);
        shader_file.close();

        auto module_info =
            vk::ShaderModuleCreateInfo{}.setCode(buffer).setCodeSize(code_size);

        return device.createShaderModule(module_info);
    }
} // namespace Rune::Vulkan
