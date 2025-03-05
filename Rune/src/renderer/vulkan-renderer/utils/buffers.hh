#pragma once

#include <cstddef>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

#include "vma.hh"

namespace Rune::Vulkan
{
    struct Buffer
    {
        vk::Buffer handle;
        VmaAllocation allocation;
        VmaAllocationInfo alloc_info;

        [[nodiscard]] static Buffer
        create(VmaAllocator allocator, std::size_t allocation_size,
               vk::BufferUsageFlags usage,
               VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO);

        void destroy();

    private:
        VmaAllocator allocator_;
    };
} // namespace Rune::Vulkan
