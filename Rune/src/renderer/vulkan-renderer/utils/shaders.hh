#pragma once

#include <filesystem>
#include <span>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace fs = std::filesystem;

namespace Rune::Vulkan
{
    class PipelineBuilder
    {
    public:
        PipelineBuilder(vk::Device& device, vk::PipelineLayout& layout);

        PipelineBuilder& set_shader(vk::ShaderStageFlagBits stage,
                                    vk::ShaderModule shader,
                                    const char* entrypoint = "main");

        PipelineBuilder& set_input_topology(vk::PrimitiveTopology topology,
                                            bool primitive_restart = false);

        PipelineBuilder& set_polygon_mode(vk::PolygonMode mode,
                                          float line_width = 1.f);

        PipelineBuilder& set_culling(vk::CullModeFlags cull_mode,
                                     vk::FrontFace front_face);

        PipelineBuilder& disable_multisampling();

        PipelineBuilder& disable_blending();

        PipelineBuilder& disable_depth_test();

        PipelineBuilder& set_color_format(vk::Format format);
        PipelineBuilder& set_depth_format(vk::Format format);

        PipelineBuilder& add_dynamic_state(vk::DynamicState state);

        void set_dynamic_states(std::span<vk::DynamicState> states);

        [[nodiscard]] std::optional<vk::Pipeline> build();

        void clear();

        void clear_states();

        void clear_stages()
        {
            shader_stages_.clear();
        }

    private:
        vk::Device device_;

        vk::PipelineLayout pipeline_layout_;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_;
        vk::PipelineRasterizationStateCreateInfo rasterizer_;
        vk::PipelineColorBlendAttachmentState color_blend_attachment_;
        vk::PipelineMultisampleStateCreateInfo multisampling_;
        vk::PipelineDepthStencilStateCreateInfo depth_stencil_;
        vk::PipelineRenderingCreateInfo render_info_;
        vk::Format color_attachment_format_;

        std::unordered_set<vk::DynamicState> dynamic_states_;
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages_;
    };

    std::optional<vk::ShaderModule> load_shader(const fs::path& shader_path,
                                                vk::Device device);
} // namespace Rune::Vulkan
