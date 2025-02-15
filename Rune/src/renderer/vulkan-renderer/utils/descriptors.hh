#pragma once

#include <vulkan/vulkan.hpp>

#include "utils/types.hh"

namespace Rune::Vulkan
{
    class DescriptorPool
    {
    public:
        // NOTE: This is from the VkGuide still, not sure about the ratio
        struct SetInfo
        {
            vk::DescriptorType type;
            float ratio;
        };

        constexpr DescriptorPool()
        {}

        void device_set(vk::Device device)
        {
            device_ = device;
        }

        void init(u32 max_sets, std::span<SetInfo> infos);

        void reset();

        void destroy();

        vk::DescriptorSet allocate(std::span<vk::DescriptorSetLayout> layouts);

    private:
        vk::Device device_;
        vk::DescriptorPool pool_;
    };

    struct DescriptorLayoutBuilder
    {
        DescriptorLayoutBuilder(vk::Device device)
            : device{ device }
            , bindings{}
        {}

        vk::DescriptorSetLayout
        build(vk::ShaderStageFlags shader_stages, void* pNext = nullptr,
              vk::DescriptorSetLayoutCreateFlags create_flags = {});

        void bind(u32 binding, vk::DescriptorType type)
        {
            bindings.emplace_back(binding, type, 1);
        }

        vk::Device device;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
} // namespace Rune::Vulkan
