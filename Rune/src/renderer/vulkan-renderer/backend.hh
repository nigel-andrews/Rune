#pragma once

#include <glm/vec4.hpp>
#include <vulkan/vulkan.hpp>

#include "core/memory/deletion_stack.hh"
#include "platform/window.hh"
#include "renderer/render_backend.hh"
#include "structs.hh"
#include "utils/descriptors.hh"
#include "utils/image.hh"
#include "utils/vma.hh"

namespace Rune::Vulkan
{
#ifndef MAX_IN_FLIGHT
    constexpr auto MAX_IN_FLIGHT = 2;
#endif

    // FIXME: these are entirely vkguide specific and to be removed
    struct ComputePushConstants
    {
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };

    struct ComputeEffect
    {
        const char* name;

        vk::Pipeline pipeline;
        vk::PipelineLayout layout;

        ComputePushConstants data;
    };

    class Backend final : public RenderBackend
    {
    public:
        void init(Window* window, std::string_view app_name, i32 width,
                  i32 height) final;

        void init_imgui();

        void test_imgui() final;

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
        void init_descriptors();
        void init_pipelines();

        void shutdown_imgui();

        // FIXME: from vkguide
        void init_background_pipelines();
        void init_triangle_pipeline();

        void imgui_backend_frame(vk::CommandBuffer command, vk::ImageView view);

        void draw_background(vk::CommandBuffer command);
        void draw_geometry(vk::CommandBuffer command);

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

        // maybe a map to be able to distinguish layouts
        std::vector<vk::DescriptorSetLayout> draw_image_descriptor_layouts_;
        vk::DescriptorSet draw_image_descriptors_;

        // FIXME: from vkguide
        //
        vk::PipelineLayout gradient_pipeline_layout_;

        vk::PipelineLayout triangle_layout_;
        vk::Pipeline triangle_pipeline_;

        std::vector<ComputeEffect> background_effects_;
        int current_effect_;

        Image draw_image_;
        vk::Extent2D draw_image_extent_;

        vk::DebugUtilsMessengerEXT debug_messenger_;
        std::array<RenderData, MAX_IN_FLIGHT> frames_;
        u64 current_frame_;
        bool initialized_ = false;
        bool imgui_initialized_ = false;
    };
} // namespace Rune::Vulkan
