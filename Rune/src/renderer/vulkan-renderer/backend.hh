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
        void imgui_backend_frame() final;
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
        VulkanImage draw_image_;
        vk::Extent2D draw_image_extent_;

        vk::DebugUtilsMessengerEXT debug_messenger_;
        std::array<RenderData, MAX_IN_FLIGHT> frames_;
        u64 current_frame_;
        bool initialized_ = false;
        bool imgui_initialized_ = false;
    };
} // namespace Rune
