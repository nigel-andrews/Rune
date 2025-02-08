#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "platform/window.hh"
#include "renderer/render_backend.hh"

namespace Rune
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

    struct RenderData
    {
        vk::Semaphore swapchain_semaphore;
        vk::Semaphore render_semaphore;
        vk::Fence render_fence;
        vk::CommandPool command_pool;
        vk::CommandBuffer primary_buffer;
    };

    struct VulkanImage
    {
        vk::Image image;
        vk::ImageView image_view;
        vk::Extent3D image_extent;
        vk::Format image_format;
        VmaAllocation allocation;
    };

#ifndef MAX_IN_FLIGHT
    constexpr auto MAX_IN_FLIGHT = 2;
#endif

    class VulkanRenderer final : public RenderBackend
    {
    public:
        void init(Window* window, std::string_view app_name, i32 width,
                  i32 height) final;
        void init_imgui();
        bool is_imgui_initialized() final;
        void draw_frame() final;
        void cleanup() final;

        const RenderData& current_frame_get();

    private:
        void init_instance(std::string_view app_name);
        void create_surface();
        void select_devices();
        void check_available_queues() const;
        void get_queue_indices_and_queue();
        void init_allocator();
        void init_swapchain(i32 width, i32 height);
        void create_command_pools_and_buffers();
        void init_sync_structs();
        void init_descriptors();
        void init_pipelines();
        void init_background_pipelines();

        void imgui_backend_frame(vk::CommandBuffer command, vk::ImageView view);

    private:
        Window* window_;

        vk::Instance instance_;
        vk::PhysicalDevice gpu_;
        vk::Device device_;
        vk::SwapchainKHR swapchain_;
        VkFormat swapchain_image_format_;
        vk::Extent2D swapchain_extent_;
        std::vector<VkImage> swapchain_images_;
        std::vector<VkImageView> swapchain_images_view_;
        vk::SurfaceKHR surface_;
        vk::Queue queue_;
        vk::DescriptorPool imgui_descriptor_pool_;
        VmaAllocator allocator_ = VK_NULL_HANDLE;
        DescriptorPool pool_;
        // maybe a map to be able to distinguish layouts
        std::vector<vk::DescriptorSetLayout> draw_image_descriptor_layouts_;
        vk::DescriptorSet draw_image_descriptors_;

        // FIXME: from vkguide
        //
        vk::Pipeline gradient_pipeline_;
        vk::PipelineLayout gradient_pipeline_layout_;
        VulkanImage draw_image_;
        vk::Extent2D draw_image_extent_;

        vk::DebugUtilsMessengerEXT debug_messenger_;
        std::array<RenderData, MAX_IN_FLIGHT> frames_;
        u64 current_frame_;
        bool initialized_ = false;
        bool imgui_initialized_ = false;
    };
} // namespace Rune
