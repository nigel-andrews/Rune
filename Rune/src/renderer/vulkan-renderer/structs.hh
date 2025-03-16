#pragma once

#include <vulkan/vulkan.hpp>

#include "utils/types.hh"

namespace Rune::Vulkan
{
    // FIXME: split this in multiple headers
    struct QueueFamilies
    {
        u32 graphics;
        u32 present;
    };

    struct RenderData
    {
        vk::Semaphore swapchain_semaphore;
        vk::Semaphore render_semaphore;
        vk::Fence render_fence;
        vk::CommandPool command_pool;
        vk::CommandBuffer primary_buffer;
    };

    struct Pipeline
    {
        vk::Pipeline handle;
        vk::PipelineLayout layout;

        constexpr operator vk::Pipeline()
        {
            return handle;
        }
    };
} // namespace Rune::Vulkan
