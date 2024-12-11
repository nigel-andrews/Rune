#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "platform/window.hh"
#include "renderer/render_backend.hh"

namespace Rune
{
    struct RenderData
    {
        vk::CommandPool command_pool;
        vk::CommandBuffer primary_buffer;
    };

#ifndef MAX_IN_FLIGHT
    constexpr auto MAX_IN_FLIGHT = 2;
#endif

    class VulkanRenderer final : public RenderBackend
    {
    public:
        void init(Window* window, std::string_view app_name, i32 width,
                  i32 height) final;
        void draw_frame() final;
        void cleanup() final;

        const RenderData& current_frame_get();

    private:
        void init_instance(std::string_view app_name);
        void create_surface();
        void select_devices();
        void check_available_queues() const;
        void create_command_pools_and_buffers();

        Window* window_;

        vk::Instance instance_;
        vk::PhysicalDevice gpu_;
        vk::Device device_;
        vk::SurfaceKHR surface_;

        vk::DebugUtilsMessengerEXT debug_messenger_;
        std::array<RenderData, MAX_IN_FLIGHT> frames_;
        u64 current_frame_;
        bool initialized_ = false;
    };
} // namespace Rune
