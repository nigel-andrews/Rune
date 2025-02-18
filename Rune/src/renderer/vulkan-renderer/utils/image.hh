#pragma once

#include <vulkan/vulkan.hpp>

#include "vma.hh"

namespace Rune::Vulkan
{
    struct Image
    {
        vk::Image image;
        vk::ImageView image_view;
        vk::Extent3D image_extent;
        vk::Format image_format;
        VmaAllocation allocation;
    };

    vk::ImageSubresourceRange
    image_subresource_range(vk::ImageAspectFlags aspect_mask);

    void transition_image(vk::CommandBuffer command, VkImage image,
                          vk::ImageLayout current_layout,
                          vk::ImageLayout new_layout);

    void copy_images(vk::CommandBuffer command, vk::Image src, vk::Image dst,
                     vk::Extent2D src_extent, vk::Extent2D dst_extent);
} // namespace Rune::Vulkan
