#include "shaders.hh"

#include <fstream>

#include "core/logger/logger.hh"
#include "utils/types.hh"

namespace Rune::Vulkan
{
    std::optional<vk::ShaderModule> load_shader(const fs::path& shader_path,
                                                vk::Device device)
    {
        std::ifstream shader_file(shader_path,
                                  std::ios::ate | std::ios::binary);

        if (!shader_file.is_open())
        {
            Logger::log(Logger::WARN, "Failed to open file at ", shader_path);
            return std::nullopt;
        }

        u64 code_size = shader_file.tellg();

        std::vector<u32> buffer(code_size / sizeof(u32));

        shader_file.seekg(0);
        shader_file.read((char*)buffer.data(), code_size);
        shader_file.close();

        auto module_info =
            vk::ShaderModuleCreateInfo{}.setCode(buffer).setCodeSize(code_size);

        return device.createShaderModule(module_info);
    }

    PipelineBuilder::PipelineBuilder(vk::Device& device,
                                     vk::PipelineLayout& layout)
        : device_{ device }
        , pipeline_layout_{ layout }
        , input_assembly_{}
        , rasterizer_{}
        , color_blend_attachment_{}
        , multisampling_{}
        , depth_stencil_{}
        , render_info_{}
        , color_attachment_format_{}
    {}

    std::optional<vk::Pipeline> PipelineBuilder::build()
    {
        auto viewport_state = vk::PipelineViewportStateCreateInfo{}
                                  .setViewportCount(1)
                                  .setScissorCount(1);

        auto color_blend_state = vk::PipelineColorBlendStateCreateInfo{}
                                     .setLogicOpEnable(false)
                                     .setLogicOp(vk::LogicOp::eCopy)
                                     .setAttachmentCount(1)
                                     .setAttachments(color_blend_attachment_);

        auto vertex_input_state = vk::PipelineVertexInputStateCreateInfo{};

        std::array states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        auto dynamic_state =
            vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(states);

        if (shader_stages_.empty())
            Logger::log(Logger::WARN, "Pipeline contains no shader stages...");

        auto graphics_pipeline_create_info =
            vk::GraphicsPipelineCreateInfo{}
                .setLayout(pipeline_layout_)
                .setStages(shader_stages_)
                .setPInputAssemblyState(&input_assembly_)
                .setPRasterizationState(&rasterizer_)
                .setPColorBlendState(&color_blend_state)
                .setPViewportState(&viewport_state)
                .setPVertexInputState(&vertex_input_state)
                .setPMultisampleState(&multisampling_)
                .setPDepthStencilState(&depth_stencil_)
                .setPNext(&render_info_)
                .setPDynamicState(&dynamic_state);

        if (auto [result, value] = device_.createGraphicsPipeline(
                nullptr, graphics_pipeline_create_info);
            result == vk::Result::eSuccess)
            return value;

        Logger::error("Failed to create graphics pipeline !");

        return std::nullopt;
    }

    PipelineBuilder& PipelineBuilder::set_shader(vk::ShaderStageFlagBits stage,
                                                 vk::ShaderModule shader,
                                                 const char* entrypoint)
    {
        shader_stages_.push_back(vk::PipelineShaderStageCreateInfo{}
                                     .setStage(stage)
                                     .setModule(shader)
                                     .setPName(entrypoint));

        return *this;
    }

    PipelineBuilder&
    PipelineBuilder::set_input_topology(vk::PrimitiveTopology topology,
                                        bool primitive_restart)
    {
        input_assembly_.setTopology(topology).setPrimitiveRestartEnable(
            primitive_restart);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_polygon_mode(vk::PolygonMode mode,
                                                       float line_width)
    {
        rasterizer_.setPolygonMode(mode).setLineWidth(line_width);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_culling(vk::CullModeFlags cull_mode,
                                                  vk::FrontFace front_face)
    {
        rasterizer_.setCullMode(cull_mode).setFrontFace(front_face);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::disable_multisampling()
    {
        multisampling_.setSampleShadingEnable(false)
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setPSampleMask(nullptr)
            .setAlphaToOneEnable(false);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::disable_blending()
    {
        color_blend_attachment_
            .setColorWriteMask(vk::ColorComponentFlagBits::eR
                               | vk::ColorComponentFlagBits::eG
                               | vk::ColorComponentFlagBits::eB
                               | vk::ColorComponentFlagBits::eA)
            .setBlendEnable(false);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::disable_depth_test()
    {
        depth_stencil_.setDepthTestEnable(false)
            .setDepthCompareOp(vk::CompareOp::eNever)
            .setDepthBoundsTestEnable(false)
            .setDepthWriteEnable(false)
            .setStencilTestEnable(false)
            .setFront({})
            .setBack({})
            .setMinDepthBounds(0.f)
            .setMaxDepthBounds(1.f);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_color_format(vk::Format format)
    {
        color_attachment_format_ = format;
        render_info_.setColorAttachmentFormats(color_attachment_format_);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_depth_format(vk::Format format)
    {
        render_info_.setDepthAttachmentFormat(format);
        return *this;
    }

    void PipelineBuilder::clear()
    {
        pipeline_layout_ = vk::PipelineLayout{};

        input_assembly_ = vk::PipelineInputAssemblyStateCreateInfo{};
        rasterizer_ = vk::PipelineRasterizationStateCreateInfo{};
        color_blend_attachment_ = vk::PipelineColorBlendAttachmentState{};
        multisampling_ = vk::PipelineMultisampleStateCreateInfo{};
        depth_stencil_ = vk::PipelineDepthStencilStateCreateInfo{};
        render_info_ = vk::PipelineRenderingCreateInfo{};
        color_attachment_format_ = vk::Format{};

        shader_stages_.clear();
    }
} // namespace Rune::Vulkan
