#pragma once

#include <glm/vec4.hpp>
#include <vulkan/vulkan.hpp>

#include "core/memory/deletion_stack.hh"
#include "platform/window.hh"
#include "renderer/render_backend.hh"
#include "renderer/renderer.hh"
#include "structs.hh"
#include "utils/descriptors.hh"
#include "utils/image.hh"
#include "utils/vma.hh"

namespace Rune::Vulkan
{
#ifndef MAX_IN_FLIGHT
    constexpr auto MAX_IN_FLIGHT = 2;
#endif

    class Backend final : public RenderBackend
    {
    public:
        void init(Window* window, std::string_view app_name, i32 width,
                  i32 height) final;

        RenderBackendType type() final
        {
            return RenderBackendType::VULKAN;
        }

        void init_imgui();

        bool is_imgui_initialized() final
        {
            return imgui_initialized_;
        }

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
        void init_pipelines_descriptors();
        void init_default_pipelines();

        void shutdown_imgui();

        void imgui_backend_frame(vk::CommandBuffer command, vk::ImageView view);

        void draw_graphics(vk::CommandBuffer command);
        void transfer_to_swapchain(vk::CommandBuffer command,
                                   u32 swapchain_index);
        void submit_command(vk::CommandBuffer command,
                            const RenderData& current_frame);

    private:
        Window* window_;
        DeletionStack deletion_stack_;

        vk::Instance instance_;
        vk::PhysicalDevice gpu_;
        vk::Device device_;
        vk::SwapchainKHR swapchain_;
        VkFormat swapchain_image_format_;
        vk::Extent2D swapchain_extent_;

        // These are VkImage since Vkb returns it as such
        std::vector<VkImage> swapchain_images_;
        std::vector<VkImageView> swapchain_images_view_;

        vk::SurfaceKHR surface_;
        vk::Queue queue_;
        vk::DescriptorPool imgui_descriptor_pool_;

        VmaAllocator allocator_ = VK_NULL_HANDLE;
        DescriptorPool pool_;

        // Basic pipeline for mesh data
        Pipeline default_graphics_pipeline_;

        Image draw_image_;
        vk::ImageLayout current_draw_image_layout_;

        vk::DebugUtilsMessengerEXT debug_messenger_;
        std::array<RenderData, MAX_IN_FLIGHT> frames_;
        u64 current_frame_;
        bool initialized_ = false;
        bool imgui_initialized_ = false;
    };
} // namespace Rune::Vulkan
