#pragma once

#include <vulkan/vulkan.hpp>

namespace Rune::Vulkan
{
    vk::RenderingAttachmentInfo
    attachment_info(vk::ImageView image_view, vk::ImageLayout layout,
                    std::optional<vk::ClearValue> clear_value = std::nullopt);

    vk::RenderingInfo rendering_info(
        vk::Extent2D render_extent,
        const vk::RenderingAttachmentInfo& color_attachment,
        const vk::RenderingAttachmentInfo* depth_attachment = nullptr,
        const vk::RenderingAttachmentInfo* stencil_attachment = nullptr);

    vk::DescriptorPool init_imgui_descriptors(vk::Device device);
} // namespace Rune::Vulkan
