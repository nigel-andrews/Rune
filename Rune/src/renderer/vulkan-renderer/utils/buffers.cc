#include "buffers.hh"

#include "core/logger/logger.hh"

namespace Rune::Vulkan
{
    Buffer Buffer::create(VmaAllocator allocator, std::size_t allocation_size,
                          vk::BufferUsageFlags usage,
                          VmaMemoryUsage memory_usage)
    {
        auto create_info = static_cast<VkBufferCreateInfo>(
            vk::BufferCreateInfo{}.setSize(allocation_size).setUsage(usage));

        VmaAllocationCreateInfo allocation_info{};
        allocation_info.usage = memory_usage;
        allocation_info.flags = 0;

        Buffer buffer{};

        VkBuffer allocated_buffer;
        if (vmaCreateBuffer(allocator, &create_info, &allocation_info,
                            &allocated_buffer, &buffer.allocation,
                            &buffer.alloc_info)
            != VK_SUCCESS)
            Logger::error("Failed to create buffer", true);

        buffer.allocator_ = allocator;

        buffer.handle = allocated_buffer;

        return buffer;
    }

    void Buffer::destroy()
    {
        vmaDestroyBuffer(allocator_, handle, allocation);
    }
} // namespace Rune::Vulkan
