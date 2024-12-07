#pragma once

#include <vulkan/vulkan.hpp>

#include "renderer/render_backend.hh"

namespace Rune
{
    class VulkanRenderer final : public RenderBackend
    {
    public:
        void init() final;
        void draw_frame() final;
        void cleanup() final;

    private:
        vk::Instance instance_;
        vk::PhysicalDevice physical_device_;
        vk::Device device_;
    };
} // namespace Rune
