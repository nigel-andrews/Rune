#include "image.hh"

namespace Rune::Vulkan
{
    // Thanks https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop_code/
    vk::ImageSubresourceRange
    image_subresource_range(vk::ImageAspectFlags aspect_mask)
    {
        // WARN: Default for now from VkGuide
        vk::ImageSubresourceRange sub_image{};

        // TODO: check validity of this
        return sub_image.setAspectMask(aspect_mask)
            .setLevelCount(VK_REMAINING_MIP_LEVELS)
            .setLayerCount(VK_REMAINING_ARRAY_LAYERS);
    }

    vk::ImageLayout transition_image(vk::CommandBuffer command, vk::Image image,
                                     const ImageTransitionInfo& transition_info)
    {
        vk::ImageMemoryBarrier2 image_barrier{};

        // TODO: have access mask in transition_info
        image_barrier.setSrcStageMask(transition_info.src_stage)
            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
            .setDstStageMask(transition_info.dst_stage)
            .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite
                              | vk::AccessFlagBits2::eMemoryRead)
            .setOldLayout(transition_info.old_layout)
            .setNewLayout(transition_info.new_layout)
            .setSubresourceRange(image_subresource_range(vk::ImageAspectFlags{
                transition_info.new_layout
                        == vk::ImageLayout::eDepthAttachmentOptimal
                    ? vk::ImageAspectFlagBits::eDepth
                    : vk::ImageAspectFlagBits::eColor }))
            .setImage(image);

        vk::DependencyInfo dep_info{};
        command.pipelineBarrier2(
            dep_info.setImageMemoryBarrierCount(1).setImageMemoryBarriers(
                image_barrier));

        return transition_info.new_layout;
    }

    // If both extents are equal, maybe use copy instead of blit ?
    void copy_images(vk::CommandBuffer command, vk::Image src, vk::Image dst,
                     vk::Extent2D src_extent, vk::Extent2D dst_extent)
    {
        vk::ImageBlit2 blit{ vk::ImageSubresourceLayers{
                                 vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                             {},
                             vk::ImageSubresourceLayers{
                                 vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                             {} };

        auto [src_x, src_y] = src_extent;
        blit.srcOffsets[1] = vk::Offset3D(src_x, src_y, 1);

        auto [dst_x, dst_y] = dst_extent;
        blit.dstOffsets[1] = vk::Offset3D(dst_x, dst_y, 1);

        vk::BlitImageInfo2 blit_info{ src,
                                      vk::ImageLayout::eTransferSrcOptimal,
                                      dst,
                                      vk::ImageLayout::eTransferDstOptimal,
                                      1,
                                      &blit,
                                      vk::Filter::eLinear };

        command.blitImage2(&blit_info);
    }
} // namespace Rune::Vulkan
