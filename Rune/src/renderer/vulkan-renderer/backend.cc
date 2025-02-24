#include "backend.hh"

#include <cmath>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "renderer/vulkan-renderer/utils/initializers.hh"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "bootstrap.hh"
#include "core/logger/logger.hh"
#include "utils/defines.hh"
#include "utils/image.hh"
#include "utils/shaders.hh"

namespace fs = std::filesystem;

namespace Rune::Vulkan
{
    namespace
    {
#if defined(DEBUG) || !defined(NDEBUG)
        constexpr bool enable_validation_layers = true;
#else
        constexpr bool enable_validation_layers = false;
#endif

        constexpr auto TIMEOUT = 1000000000;

        BootstrapContext context;
        QueueFamilies queue_family_indices;
    } // namespace

    const RenderData& Backend::current_frame_get()
    {
        return frames_[current_frame_ % MAX_IN_FLIGHT];
    }

    void Backend::init(Window* window, std::string_view app_name, i32 width,
                       i32 height)
    {
        if (initialized_)
        {
            Logger::log(
                Logger::WARN,
                "Attempting to initialize already initialized VulkanRenderer");
            return;
        }

        Logger::log(Logger::INFO, "Initializing VulkanRenderer");

        window_ = window;

        init_instance(app_name);
        create_surface();
        select_devices();
        check_available_queues();
        get_queue_indices_and_queue();
        init_allocator();
        init_swapchain(width, height);
        create_command_pools_and_buffers();
        init_sync_structs();
        init_descriptors();
        init_pipelines();

        initialized_ = true;

        init_imgui();
    }

    void Backend::init_imgui()
    {
        ImGui_ImplGlfw_InitForVulkan(window_->get(), true);

        imgui_descriptor_pool_ = init_imgui_descriptors(device_);

        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = instance_;
        info.PhysicalDevice = gpu_;
        info.Device = device_;
        info.QueueFamily = queue_family_indices.graphics;
        info.Queue = queue_;
        info.PipelineCache = VK_NULL_HANDLE;
        info.DescriptorPool = imgui_descriptor_pool_;
        info.MinImageCount = 3;
        info.ImageCount = 3;
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.UseDynamicRendering = true;

        info.PipelineRenderingCreateInfo = {};
        info.PipelineRenderingCreateInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineRenderingCreateInfo.pColorAttachmentFormats =
            &swapchain_image_format_;

        ImGui_ImplVulkan_Init(&info);
        ImGui_ImplVulkan_CreateFontsTexture();

        imgui_initialized_ = true;
        Logger::log(Logger::INFO, "ImGui initialized for Vulkan backend");
    }

    // FIXME: Try to handle this drawing outside of the main draw_frame
    // (threads ? different queues ?)
    void Backend::imgui_backend_frame(vk::CommandBuffer command,
                                      vk::ImageView view)
    {
        ImGui_ImplVulkan_NewFrame();

        auto color_attachment =
            attachment_info(view, vk::ImageLayout::eColorAttachmentOptimal);
        auto render_info = rendering_info(swapchain_extent_, color_attachment);

        command.beginRendering(render_info);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);

        command.endRendering();
    }

    // FIXME: this should be handled indepedently from the render backend (in
    // Gui)
    void Backend::test_imgui()
    {
        if (ImGui::Begin("background"))
        {
            ComputeEffect& selected = background_effects_[current_effect_];

            ImGui::Text("Selected effect: %s", selected.name);

            ImGui::SliderInt("Effect Index", &current_effect_, 0,
                             background_effects_.size() - 1);

            ImGui::InputFloat4("data1", (float*)&selected.data.data1);
            ImGui::InputFloat4("data2", (float*)&selected.data.data2);
            ImGui::InputFloat4("data3", (float*)&selected.data.data3);
            ImGui::InputFloat4("data4", (float*)&selected.data.data4);
        }
        else
        {
            Logger::log(Logger::WARN, "ImGui::Begin background window failed");
        }
        ImGui::End();
    }

    void Backend::draw_background(vk::CommandBuffer command)
    {
        auto& effect = background_effects_[current_effect_];

        command.bindPipeline(vk::PipelineBindPoint::eCompute, effect.pipeline);

        command.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                   gradient_pipeline_layout_, 0,
                                   draw_image_descriptors_, nullptr);

        command.pushConstants(gradient_pipeline_layout_,
                              vk::ShaderStageFlagBits::eCompute, 0,
                              sizeof(ComputePushConstants), &effect.data);

        command.dispatch(std::ceil(draw_image_extent_.width / 16.),
                         std::ceil(draw_image_extent_.height / 16.), 1);
    }

    void Backend::draw_geometry(vk::CommandBuffer command)
    {
        auto render_attach_info = attachment_info(
            draw_image_.image_view, vk::ImageLayout::eColorAttachmentOptimal);

        auto render_info =
            rendering_info(draw_image_extent_, render_attach_info);

        command.beginRendering(render_info);

        command.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             triangle_pipeline_);

        auto viewport = vk::Viewport{}
                            .setX(0.f)
                            .setY(0.f)
                            .setWidth(draw_image_extent_.width)
                            .setHeight(draw_image_extent_.height)
                            .setMinDepth(0.f)
                            .setMaxDepth(1.f);

        command.setViewport(0, 1, &viewport);

        auto scissor = vk::Rect2D{}.setExtent(draw_image_extent_).setOffset({});

        command.setScissor(0, scissor);

        command.draw(3, 1, 0, 0);

        command.endRendering();
    }

    void Backend::draw_frame()
    {
        auto& current_frame = current_frame_get();
        while (
            device_.waitForFences(current_frame.render_fence, VK_TRUE, TIMEOUT)
            == vk::Result::eTimeout)
            continue;

        auto [result, swapchain_image_index] = device_.acquireNextImageKHR(
            swapchain_, TIMEOUT, current_frame.swapchain_semaphore);

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            Logger::error("Image acquisition failed");
            return;
        }

        device_.resetFences(current_frame.render_fence);

        // XXX: try a ref
        vk::CommandBuffer command = current_frame.primary_buffer;
        command.reset();

        // FIXME: Right now, simply following VkGuide, integrate customisable
        // draws later

        // NOTE: One time submit -> each frame the buffer is submitted once then
        // reset so might speedup
        command.begin(vk::CommandBufferBeginInfo{}.setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        transition_image(command, draw_image_.image,
                         vk::ImageLayout::eUndefined,
                         vk::ImageLayout::eGeneral);

        draw_background(command);

        transition_image(command, draw_image_.image, vk::ImageLayout::eGeneral,
                         vk::ImageLayout::eColorAttachmentOptimal);

        draw_geometry(command);

        transition_image(command, draw_image_.image,
                         vk::ImageLayout::eColorAttachmentOptimal,
                         vk::ImageLayout::eTransferSrcOptimal);

        auto swapchain_image = swapchain_images_[swapchain_image_index];

        transition_image(command, swapchain_image, vk::ImageLayout::eUndefined,
                         vk::ImageLayout::eTransferDstOptimal);

        copy_images(command, draw_image_.image, swapchain_image,
                    draw_image_extent_, swapchain_extent_);

        if (imgui_initialized_)
        {
            transition_image(command, swapchain_image,
                             vk::ImageLayout::eTransferDstOptimal,
                             vk::ImageLayout::eColorAttachmentOptimal);
            imgui_backend_frame(command,
                                swapchain_images_view_[swapchain_image_index]);
            transition_image(command, swapchain_image,
                             vk::ImageLayout::eColorAttachmentOptimal,
                             vk::ImageLayout::ePresentSrcKHR);
        }
        else
            transition_image(command, swapchain_image,
                             vk::ImageLayout::eTransferDstOptimal,
                             vk::ImageLayout::ePresentSrcKHR);

        command.end();

        vk::SemaphoreSubmitInfo wait_info{
            current_frame.swapchain_semaphore, 1,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        };

        vk::SemaphoreSubmitInfo signal_info{
            current_frame.render_semaphore, 1,
            vk::PipelineStageFlagBits2::eAllGraphics
        };

        vk::CommandBufferSubmitInfo command_info{ command };

        vk::SubmitInfo2 submit_info{};
        submit_info.setWaitSemaphoreInfoCount(1)
            .setWaitSemaphoreInfos(wait_info)
            .setSignalSemaphoreInfoCount(1)
            .setSignalSemaphoreInfos(signal_info)
            .setCommandBufferInfoCount(1)
            .setCommandBufferInfos(command_info);

        VKERROR(queue_.submit2(1, &submit_info, current_frame.render_fence),
                "command submission failed");

        VKERROR(queue_.presentKHR(
                    vk::PresentInfoKHR{ 1, &current_frame.render_semaphore, 1,
                                        &swapchain_, &swapchain_image_index }),
                "queue presentation failed");
        ++current_frame_;
    }

    void Backend::shutdown_imgui()
    {
        if (imgui_initialized_)
        {
            ImGui_ImplVulkan_Shutdown();
            device_.destroyDescriptorPool(imgui_descriptor_pool_);
        }
    }

    // FIXME: Handle sigint for graceful shutdown
    //
    void Backend::cleanup()
    {
        if (!initialized_)
        {
            Logger::log(Logger::WARN,
                        "Attempting to cleanup non-initialized VulkanRenderer");
            return;
        }

        Logger::log(Logger::INFO, "Cleaning up VulkanRenderer");

        device_.waitIdle();

        deletion_stack_.clear();
        swapchain_images_view_.clear();

        device_.destroySwapchainKHR(swapchain_);

        instance_.destroySurfaceKHR(surface_);

        context.dispatch.destroyDebugUtilsMessengerEXT(debug_messenger_,
                                                       nullptr);
        vmaDestroyAllocator(allocator_);

        shutdown_imgui();

        pool_.destroy();

        device_.destroy();
        instance_.destroy();
    }

    void Backend::init_instance(std::string_view app_name)
    {
        u32 count;
        auto required_extensions = glfwGetRequiredInstanceExtensions(&count);
        std::vector<const char*> extensions;
        extensions.assign(required_extensions, required_extensions + count);

#if defined(DEBUG) || !defined(NDEBUG)
        for (auto extension : extensions)
            Logger::log(Logger::Level::INFO, "Found instance extension",
                        extension);

        auto extension_properties = vk::enumerateInstanceExtensionProperties();

        for (const auto& extension_property : extension_properties)
            Logger::log(Logger::Level::INFO,
                        "Found extension_property extension",
                        extension_property.extensionName);
#endif

        vkb::InstanceBuilder builder;

        auto builder_return =
            builder.set_app_name(app_name.data())
                .require_api_version(1, 3)
                .request_validation_layers(enable_validation_layers)
                .use_default_debug_messenger() // TODO: custom debug message
                .enable_extensions(extensions)
                .build();

        context.dispatch = builder_return->make_table();

        if (!builder_return)
            Logger::abort(builder_return.error().message());

        debug_messenger_ = builder_return->debug_messenger;

        context.instance = builder_return.value();
        instance_ = context.instance.instance;

        Logger::log(Logger::INFO, "Created Vulkan instance");
    }

    void Backend::create_surface()
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VKASSERT(static_cast<vk::Result>(glfwCreateWindowSurface(
                     instance_, window_->get(), nullptr, &surface)),
                 "Failed to create window surface");
        surface_ = surface;
        Logger::log(Logger::INFO, "Created Vulkan surface");
    }

    void Backend::select_devices()
    {
        VkPhysicalDeviceVulkan13Features features13{};
        // TODO: investigate more features
        //
        // Maybe this can be retrieved via a function plugged in by the
        // client for more customisation ?
        features13.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features features12{};
        features12.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        vkb::PhysicalDeviceSelector selector{ context.instance };

        auto selected_gpu = selector.set_minimum_version(1, 3)
                                .set_surface(surface_)
                                .require_present()
                                .set_required_features_12(features12)
                                .set_required_features_13(features13)
                                .select();

        if (!selected_gpu)
            Logger::abort(selected_gpu.error().message());

        gpu_ = selected_gpu.value();

        Logger::log(Logger::INFO, "Selected GPU :", selected_gpu->name);

        vkb::DeviceBuilder builder{ selected_gpu.value() };
        context.device = builder.build().value();
        device_ = context.device.device;
        pool_.device_set(device_);

        Logger::log(Logger::INFO, "Device created");
    }

    void Backend::check_available_queues() const
    {
#if defined(DEBUG) || !defined(NDEBUG)
        auto graphics_queue =
            context.device.get_queue(vkb::QueueType::graphics);

        if (!graphics_queue) [[unlikely]]
            Logger::abort(graphics_queue.error().message());
        else [[likely]]
            Logger::log(Logger::INFO, "Found graphics queue");

        auto present_queue = context.device.get_queue(vkb::QueueType::present);

        if (!present_queue) [[unlikely]]
            Logger::abort(present_queue.error().message());
        else [[likely]]
            Logger::log(Logger::INFO, "Found present queue");
#endif
    }

    void Backend::get_queue_indices_and_queue()
    {
        auto properties = gpu_.getQueueFamilyProperties2();
        u32 i = 0;

        for (const auto& property : properties)
        {
            if (property.queueFamilyProperties.queueFlags
                & vk::QueueFlagBits::eGraphics)
                queue_family_indices.graphics = i;

            if (gpu_.getSurfaceSupportKHR(i, surface_))
                queue_family_indices.present = i;

            ++i;
        }

        queue_ = device_.getQueue2(
            vk::DeviceQueueInfo2{ {}, queue_family_indices.graphics });
    }

    void Backend::init_allocator()
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = gpu_;
        allocatorInfo.device = device_;
        allocatorInfo.instance = instance_;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &allocator_);

        if (allocator_ == VK_NULL_HANDLE)
            Logger::abort("Failed to create VMA allocator");
        else
            Logger::log(Logger::INFO, "Created Vulkan memory allocator");
    }

    void Backend::init_swapchain(i32 width, i32 height)
    {
        vkb::SwapchainBuilder builder{ gpu_, device_, surface_ };

        // XXX: learn more about formats
        swapchain_image_format_ = VK_FORMAT_B8G8R8A8_UNORM;

        auto result =
            builder
                .set_desired_format(
                    { .format = swapchain_image_format_,
                      .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
                // NOTE: FIFO <=> VSync
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                .set_desired_extent(width, height)
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .build();

        if (!result)
            Logger::abort(result.error().message());

        swapchain_ = result.value();
        swapchain_extent_ = result->extent;
        swapchain_images_ = result->get_images().value();
        swapchain_images_view_ = result->get_image_views().value();

        deletion_stack_.push([this] {
            for (auto& image_view : swapchain_images_view_)
                device_.destroyImageView(image_view);
        });

        draw_image_.image_format = vk::Format::eR16G16B16A16Sfloat;
        draw_image_.image_extent =
            vk::Extent3D(window_->width_get(), window_->height_get(), 1);
        auto [x, y, _] = draw_image_.image_extent;
        draw_image_extent_ = vk::Extent2D{ x, y };

        vk::ImageUsageFlags flags = vk::ImageUsageFlagBits::eTransferSrc
            | vk::ImageUsageFlagBits::eTransferDst
            | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eColorAttachment;

        VkImageCreateInfo image_info =
            vk::ImageCreateInfo{}
                .setImageType(vk::ImageType::e2D)
                .setFormat(draw_image_.image_format)
                .setExtent(draw_image_.image_extent)
                .setMipLevels(1)
                .setArrayLayers(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setTiling(vk::ImageTiling::eOptimal)
                .setUsage(flags);

        VmaAllocationCreateInfo image_alloc_info{};
        // NOTE: this should be AUTO
        image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        image_alloc_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        VkImage image;
        if (vmaCreateImage(allocator_, &image_info, &image_alloc_info, &image,
                           &draw_image_.allocation, nullptr)
            != VK_SUCCESS)
            Logger::abort("Failed to create draw image");

        draw_image_.image = image;

        deletion_stack_.push([this] {
            vmaDestroyImage(allocator_, draw_image_.image,
                            draw_image_.allocation);
        });

        vk::ImageViewCreateInfo view_info{};
        view_info.setImage(draw_image_.image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(draw_image_.image_format)
            .subresourceRange.setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);

        VKASSERT(device_.createImageView(&view_info, nullptr,
                                         &draw_image_.image_view),
                 "Failed to create image view");

        deletion_stack_.push(
            [this] { device_.destroyImageView(draw_image_.image_view); });

        Logger::log(Logger::INFO, "Created swapchain and draw image");
    }

    void Backend::create_command_pools_and_buffers()
    {
        vk::CommandPoolCreateInfo create_info{
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            queue_family_indices.graphics
        };

        for (auto& frame : frames_)
        {
            frame.command_pool = device_.createCommandPool(create_info);
            frame.primary_buffer =
                device_
                    .allocateCommandBuffers(vk::CommandBufferAllocateInfo(
                        frame.command_pool, vk::CommandBufferLevel::ePrimary,
                        1))
                    .front();
        }

        Logger::log(Logger::INFO, "Created command pools and buffers");
    }

    void Backend::init_sync_structs()
    {
        vk::FenceCreateInfo fence_create_info{
            vk::FenceCreateFlagBits::eSignaled
        };

        vk::SemaphoreCreateInfo semaphore_create_info{};

        for (auto& frame : frames_)
        {
            frame.render_fence = device_.createFence(fence_create_info);
            frame.swapchain_semaphore =
                device_.createSemaphore(semaphore_create_info);
            frame.render_semaphore =
                device_.createSemaphore(semaphore_create_info);

            // Individual functors or one with a loop ?
            deletion_stack_.push([this, &frame] {
                device_.destroyCommandPool(frame.command_pool);
                device_.destroyFence(frame.render_fence);
                device_.destroySemaphore(frame.render_semaphore);
                device_.destroySemaphore(frame.swapchain_semaphore);
            });
        }
    }

    void Backend::init_descriptors()
    {
        std::vector<DescriptorPool::SetInfo> set_infos{
            { vk::DescriptorType::eStorageImage, 1 },
        };

        // Device in pool is set when it is selected
        pool_.init(10, set_infos);

        {
            DescriptorLayoutBuilder builder{ device_ };
            builder.bind(0, vk::DescriptorType::eStorageImage);
            draw_image_descriptor_layouts_.push_back(
                builder.build(vk::ShaderStageFlagBits::eCompute));
        }

        draw_image_descriptors_ =
            pool_.allocate(draw_image_descriptor_layouts_);

        auto image_info = vk::DescriptorImageInfo{}
                              .setImageLayout(vk::ImageLayout::eGeneral)
                              .setImageView(draw_image_.image_view);

        auto write_descriptor_set =
            vk::WriteDescriptorSet{}
                .setDstBinding(0)
                .setDstSet(draw_image_descriptors_)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setImageInfo(image_info);

        device_.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);

        deletion_stack_.push([this] {
            for (auto layout : draw_image_descriptor_layouts_)
                device_.destroyDescriptorSetLayout(layout);
        });
    }

    void Backend::init_pipelines()
    {
        init_background_pipelines();
        init_triangle_pipeline();
    }

    void Backend::init_triangle_pipeline()
    {
        auto triangle_frag = load_shader(
            "build/debug/Rune/src/renderer/shaders/triangle.frag.spv", device_);

        if (!triangle_frag)
            Logger::abort("failed to init triangle frag");

        auto triangle_vert = load_shader(
            "build/debug/Rune/src/renderer/shaders/triangle.vert.spv", device_);

        if (!triangle_vert)
            Logger::abort("failed to init triangle vert");

        auto pipeline_layout = vk::PipelineLayoutCreateInfo{};

        triangle_layout_ = device_.createPipelineLayout(pipeline_layout);

        auto builder =
            PipelineBuilder{ device_, triangle_layout_ }
                .set_shader(vk::ShaderStageFlagBits::eVertex, *triangle_vert)
                .set_shader(vk::ShaderStageFlagBits::eFragment, *triangle_frag)
                .set_input_topology(vk::PrimitiveTopology::eTriangleList)
                .set_polygon_mode(vk::PolygonMode::eFill)
                .set_culling(vk::CullModeFlagBits::eNone,
                             vk::FrontFace::eClockwise)
                .disable_multisampling()
                .disable_depth_test()
                .disable_blending()
                .set_color_format(draw_image_.image_format)
                .set_depth_format(vk::Format::eUndefined);

        auto result =
            builder.build().or_else([] -> std::optional<vk::Pipeline> {
                Logger::error("Failed to create pipeline");
                return std::nullopt;
            });

        triangle_pipeline_ = result.value_or(VK_NULL_HANDLE);

        device_.destroyShaderModule(*triangle_vert);
        device_.destroyShaderModule(*triangle_frag);

        deletion_stack_.push([this] {
            device_.destroyPipelineLayout(triangle_layout_);
            device_.destroyPipeline(triangle_pipeline_);
        });

        Logger::log(Logger::INFO, "Initialized triangle pipeline");
    }

    void Backend::init_background_pipelines()
    {
        auto range = vk::PushConstantRange{}
                         .setStageFlags(vk::ShaderStageFlagBits::eCompute)
                         .setSize(sizeof(ComputePushConstants))
                         .setOffset(0);

        auto pipeline_layout_create_info =
            vk::PipelineLayoutCreateInfo{}
                .setSetLayouts(draw_image_descriptor_layouts_)
                .setPushConstantRanges(range);

        gradient_pipeline_layout_ =
            device_.createPipelineLayout(pipeline_layout_create_info);

        // FIXME: better path handling for this (maybe a define added in the
        // CMake after compiling the shader)
        fs::path gradient_shader_path =
            "build/debug/Rune/src/renderer/shaders/gradient.comp.spv";
        auto gradient_shader = load_shader(gradient_shader_path, device_);

        if (!gradient_shader)
            Logger::abort(std::format("Failed to load compute shader !"));

        auto pipeline_shader_stage_create_info =
            vk::PipelineShaderStageCreateInfo{}
                .setModule(*gradient_shader)
                .setStage(vk::ShaderStageFlagBits::eCompute)
                .setPName("main");

        auto compute_pipeline_create_info =
            vk::ComputePipelineCreateInfo{}
                .setStage(pipeline_shader_stage_create_info)
                .setLayout(gradient_pipeline_layout_);

        ComputeEffect gradient;
        gradient.layout = gradient_pipeline_layout_;
        gradient.name = "gradient";
        gradient.data = {};

        gradient.data.data1 = glm::vec4(1, 0, 0, 1);
        gradient.data.data2 = glm::vec4(0, 0, 1, 1);

        auto [gradient_result, gradient_pipeline] =
            device_.createComputePipeline(VK_NULL_HANDLE,
                                          compute_pipeline_create_info);

        if (gradient_result != vk::Result::eSuccess)
            Logger::abort("Failed to create gradient pipeline");

        gradient.pipeline = gradient_pipeline;

        device_.destroyShaderModule(*gradient_shader);

        fs::path sky_shader_path =
            "build/debug/Rune/src/renderer/shaders/sky.spv";
        auto sky_shader = load_shader(sky_shader_path, device_);

        if (!sky_shader)
            Logger::abort(std::format("Failed to load compute shader !"));

        compute_pipeline_create_info.stage.setModule(*sky_shader);

        ComputeEffect sky;
        sky.layout = gradient_pipeline_layout_;
        sky.name = "sky";
        sky.data = {};

        sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

        auto [sky_result, sky_pipeline] = device_.createComputePipeline(
            VK_NULL_HANDLE, compute_pipeline_create_info);

        if (sky_result != vk::Result::eSuccess)
            Logger::abort("Failed to create sky pipeline");

        sky.pipeline = sky_pipeline;

        device_.destroyShaderModule(*sky_shader);

        background_effects_.push_back(gradient);
        background_effects_.push_back(sky);

        deletion_stack_.push([this] {
            device_.destroyPipelineLayout(gradient_pipeline_layout_);
        });

        deletion_stack_.push([this] {
            for (auto& effect : background_effects_)
                device_.destroyPipeline(effect.pipeline);
        });
    }
} // namespace Rune::Vulkan
