#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "platform/window.hh"
#include "renderer/render_backend.hh"

namespace Rune
{
    class VulkanRenderer final : public RenderBackend
    {
    public:
        void init(Window* window, std::string_view app_name, i32 width,
                  i32 height) final;
        void draw_frame() final;
        void cleanup() final;

    private:
        void init_instance(std::string_view app_name);
        void create_surface();
        void select_devices();
        void check_available_queues() const;

        Window* window_;

        vk::Instance instance_;
        vk::PhysicalDevice gpu_;
        vk::Device device_;
        vk::SurfaceKHR surface_;
        vk::DebugUtilsMessengerEXT debug_messenger_;
    };
} // namespace Rune
