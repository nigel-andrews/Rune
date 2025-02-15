#include "descriptors.hh"

#include "defines.hh"

namespace Rune::Vulkan
{
    void DescriptorPool::init(u32 max_sets, std::span<SetInfo> infos)
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes{};

        for (auto info : infos)
            pool_sizes.emplace_back(info.type, info.ratio * max_sets);

        auto create_info =
            vk::DescriptorPoolCreateInfo{}.setMaxSets(max_sets).setPoolSizes(
                pool_sizes);
        VKWARN(device_.createDescriptorPool(&create_info, nullptr, &pool_),
               "Failed to create DescriptorPool");
    }

    void DescriptorPool::reset()
    {
        device_.resetDescriptorPool(pool_);
    }

    void DescriptorPool::destroy()
    {
        device_.destroyDescriptorPool(pool_);
    }

    vk::DescriptorSet
    DescriptorPool::allocate(std::span<vk::DescriptorSetLayout> layouts)
    {
        auto alloc_info = vk::DescriptorSetAllocateInfo{}
                              .setSetLayouts(layouts)
                              .setDescriptorPool(pool_);

        vk::DescriptorSet set{};
        VKWARN(device_.allocateDescriptorSets(&alloc_info, &set),
               "Failed to allocate descript sets");

        return set;
    }

    vk::DescriptorSetLayout DescriptorLayoutBuilder::build(
        vk::ShaderStageFlags shader_stages, void* pNext,
        vk::DescriptorSetLayoutCreateFlags create_flags)
    {
        for (auto& binding : bindings)
            binding.stageFlags |= shader_stages;

        auto create_info = vk::DescriptorSetLayoutCreateInfo{}
                               .setBindings(bindings)
                               .setFlags(create_flags)
                               .setPNext(pNext);

        vk::DescriptorSetLayout layout{};

        VKASSERT(
            device.createDescriptorSetLayout(&create_info, nullptr, &layout),
            "Failed to create desciptor set layout");

        return layout;
    }
} // namespace Rune::Vulkan
