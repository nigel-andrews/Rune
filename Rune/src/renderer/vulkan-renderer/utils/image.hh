#pragma once

#include <vulkan/vulkan.hpp>

#include "vma.hh"

namespace Rune::Vulkan
{
    struct Image
    {
        vk::Image image;
        vk::ImageView view;
        vk::Extent3D extent;
        vk::Format format;

        constexpr operator vk::Image()
        {
            return image;
        }

        VmaAllocation allocation;
    };

    struct ImageTransitionInfo
    {
        vk::PipelineStageFlags2 src_stage =
            vk::PipelineStageFlagBits2::eAllCommands;
        vk::PipelineStageFlags2 dst_stage =
            vk::PipelineStageFlagBits2::eAllCommands;
        vk::ImageLayout old_layout = vk::ImageLayout::eUndefined;
        vk::ImageLayout new_layout;
    };

    vk::ImageSubresourceRange
    image_subresource_range(vk::ImageAspectFlags aspect_mask);

    // Return the the new layout used by the image
    vk::ImageLayout
    transition_image(vk::CommandBuffer command, vk::Image image,
                     const ImageTransitionInfo& transition_info);

    void copy_images(vk::CommandBuffer command, vk::Image src, vk::Image dst,
                     vk::Extent2D src_extent, vk::Extent2D dst_extent);
} // namespace Rune::Vulkan
