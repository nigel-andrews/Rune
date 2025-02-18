#include "initializers.hh"

#include "core/logger/logger.hh"

namespace Rune::Vulkan
{
    vk::RenderingInfo
    rendering_info(vk::Extent2D render_extent,
                   const vk::RenderingAttachmentInfo& color_attachment,
                   const vk::RenderingAttachmentInfo* depth_attachment,
                   const vk::RenderingAttachmentInfo* stencil_attachment)
    {
        return vk::RenderingInfo{}
            .setRenderArea(vk::Rect2D{ vk::Offset2D{}, render_extent })
            .setLayerCount(1)
            .setColorAttachments(color_attachment)
            .setPDepthAttachment(depth_attachment)
            .setPStencilAttachment(stencil_attachment);
    }

    vk::RenderingAttachmentInfo
    attachment_info(vk::ImageView image_view, vk::ImageLayout layout,
                    std::optional<vk::ClearValue> clear_value)
    {
        auto rendering_attachment_info =
            vk::RenderingAttachmentInfo{}
                .setImageView(image_view)
                .setImageLayout(layout)
                .setLoadOp(clear_value ? vk::AttachmentLoadOp::eClear
                                       : vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

        if (clear_value)
            rendering_attachment_info.setClearValue(*clear_value);

        return rendering_attachment_info;
    }

    vk::DescriptorPool init_imgui_descriptors(vk::Device device)
    {
        static constexpr std::array pool_sizes = {
            vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 }
        };

        vk::DescriptorPoolCreateInfo pool_info{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        };

        for (const VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;

        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        vk::DescriptorPool imgui_descriptor_pool{};

        // Using VKWARN here causes validation error because of the do
        // while(false)
        if (device.createDescriptorPool(&pool_info, nullptr,
                                        &imgui_descriptor_pool)
            != vk::Result::eSuccess)
        {
            Logger::log(
                Logger::WARN,
                "Failed to initialize ImGui descriptor pools, skipping");
        }

        return imgui_descriptor_pool;
    }
} // namespace Rune::Vulkan
