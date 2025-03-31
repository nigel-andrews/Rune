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
        init_default_pipelines();
        init_pipelines_descriptors();

        initialized_ = true;
    }

    void Backend::init_gui(Gui* gui)
    {
        gui_ = gui;
        init_imgui();
    }

    void Backend::init_imgui()
    {
        if (!gui_)
            return;

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

        Logger::log(Logger::INFO, "ImGui initialized for Vulkan backend");
    }

    // FIXME: Try to handle this drawing outside of the main draw_frame
    // (threads ? different queues ?)
    void Backend::imgui_backend_frame(vk::CommandBuffer command,
                                      vk::ImageView view)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        auto color_attachment =
            attachment_info(view, vk::ImageLayout::eColorAttachmentOptimal);
        auto render_info = rendering_info(swapchain_extent_, color_attachment);

        command.beginRendering(render_info);

        gui_->draw_frame();

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);

        command.endRendering();
    }

    void Backend::draw_graphics(vk::CommandBuffer command)
    {
        auto color_attachment = attachment_info(
            draw_image_.view, vk::ImageLayout::eColorAttachmentOptimal);
        auto render_info = rendering_info(swapchain_extent_, color_attachment);

        current_draw_image_layout_ = transition_image(
            command, draw_image_,
            ImageTransitionInfo{ .old_layout = current_draw_image_layout_,
                                 .new_layout =
                                     vk::ImageLayout::eAttachmentOptimal });

        command.beginRendering(render_info);

        command.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             default_graphics_pipeline_);

        const auto viewport = vk::Viewport{}
                                  .setWidth(draw_image_.extent.width)
                                  .setHeight(draw_image_.extent.height)
                                  .setMaxDepth(1.f);

        command.setViewport(0, 1, &viewport);

        const auto scissor = vk::Rect2D{}.setOffset({}).setExtent(
            { draw_image_.extent.width, draw_image_.extent.height });

        command.setScissor(0, 1, &scissor);

        command.draw(3, 1, 0, 0);

        command.endRendering();
    }

    void Backend::transfer_to_swapchain(vk::CommandBuffer command,
                                        u32 swapchain_index)
    {
        current_draw_image_layout_ = transition_image(
            command, draw_image_,
            ImageTransitionInfo{ .old_layout = current_draw_image_layout_,
                                 .new_layout =
                                     vk::ImageLayout::eTransferSrcOptimal });

        auto swapchain_image = swapchain_images_[swapchain_index];

        transition_image(
            command, swapchain_image,
            { .old_layout = vk::ImageLayout::eUndefined,
              .new_layout = vk::ImageLayout::eTransferDstOptimal });

        copy_images(
            command, draw_image_, swapchain_image,
            vk::Extent2D{ draw_image_.extent.width, draw_image_.extent.height },
            swapchain_extent_);

        if (gui_)
        {
            transition_image(
                command, swapchain_image,
                { .old_layout = vk::ImageLayout::eTransferDstOptimal,
                  .new_layout = vk::ImageLayout::eColorAttachmentOptimal });

            imgui_backend_frame(command,
                                swapchain_images_view_[swapchain_index]);
            transition_image(
                command, swapchain_image,
                { .old_layout = vk::ImageLayout::eColorAttachmentOptimal,
                  .new_layout = vk::ImageLayout::ePresentSrcKHR });
        }
        else
            transition_image(
                command, swapchain_image,
                { .old_layout = vk::ImageLayout::eTransferDstOptimal,
                  .new_layout = vk::ImageLayout::ePresentSrcKHR });
    }

    void Backend::submit_command(vk::CommandBuffer command,
                                 const RenderData& current_frame)
    {
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

        vk::CommandBuffer command = current_frame.primary_buffer;
        command.reset();

        // NOTE: One time submit -> each frame the buffer is submitted once then
        // reset so might speedup
        // Should this be prerecorded at initialization of the command buffers ?
        command.begin(vk::CommandBufferBeginInfo{}.setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        draw_graphics(command);

        transfer_to_swapchain(command, swapchain_image_index);

        command.end();

        submit_command(command, current_frame);

        VKERROR(queue_.presentKHR(
                    vk::PresentInfoKHR{ 1, &current_frame.render_semaphore, 1,
                                        &swapchain_, &swapchain_image_index }),
                "queue presentation failed");

        ++current_frame_;
    }

    void Backend::shutdown_imgui()
    {
        if (gui_)
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

        draw_image_.format = vk::Format::eR16G16B16A16Sfloat;
        draw_image_.extent =
            vk::Extent3D(window_->width_get(), window_->height_get(), 1);

        vk::ImageUsageFlags flags = vk::ImageUsageFlagBits::eTransferSrc
            | vk::ImageUsageFlagBits::eTransferDst
            | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eColorAttachment;

        VkImageCreateInfo image_info =
            vk::ImageCreateInfo{}
                .setImageType(vk::ImageType::e2D)
                .setFormat(draw_image_.format)
                .setExtent(draw_image_.extent)
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
            .setFormat(draw_image_.format)
            .subresourceRange.setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);

        VKASSERT(
            device_.createImageView(&view_info, nullptr, &draw_image_.view),
            "Failed to create image view");

        deletion_stack_.push(
            [this] { device_.destroyImageView(draw_image_.view); });

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

    void Backend::init_pipelines_descriptors()
    {}

    void Backend::init_default_pipelines()
    {
#ifndef RUNE_VULKAN_DEFAULT_SHADER_PATH
        Logger::abort("Default shaders not found !");
#endif
        auto default_pipeline_create_info =
            vk::PipelineLayoutCreateInfo{} // TODO: input buffer in descriptors
                .setSetLayouts({});

        default_graphics_pipeline_.layout =
            device_.createPipelineLayout(default_pipeline_create_info);
        PipelineBuilder graphics_builder{ device_,
                                          default_graphics_pipeline_.layout };

        auto default_vertex_shader = load_shader(
            RUNE_VULKAN_DEFAULT_SHADER_PATH "/default.vert.spv", device_);
        if (!default_vertex_shader)
            Logger::abort("Failed to create default vertex shader !");

        graphics_builder.set_shader(vk::ShaderStageFlagBits::eVertex,
                                    *default_vertex_shader);

        auto default_frag_shader = load_shader(
            RUNE_VULKAN_DEFAULT_SHADER_PATH "/default.frag.spv", device_);
        if (!default_frag_shader)
            Logger::abort("Failed to create default fragment shader !");

        graphics_builder.set_shader(vk::ShaderStageFlagBits::eFragment,
                                    *default_frag_shader);

        graphics_builder
            .set_culling(vk::CullModeFlagBits::eFront,
                         vk::FrontFace::eCounterClockwise)
            .set_polygon_mode(vk::PolygonMode::eFill)
            .set_input_topology(vk::PrimitiveTopology::eTriangleList)
            .set_color_format(draw_image_.format)
            .set_depth_format(vk::Format::eUndefined)
            .disable_blending()
            .disable_depth_test()
            .disable_multisampling();

        auto built = graphics_builder.build();
        if (!built)
            Logger::abort("Failed to create default graphics pipeline !");

        device_.destroyShaderModule(*default_vertex_shader);
        device_.destroyShaderModule(*default_frag_shader);

        default_graphics_pipeline_.handle = *built;

        deletion_stack_.push([this] {
            device_.destroyPipeline(default_graphics_pipeline_.handle);
            device_.destroyPipelineLayout(default_graphics_pipeline_.layout);
        });
    }
} // namespace Rune::Vulkan
