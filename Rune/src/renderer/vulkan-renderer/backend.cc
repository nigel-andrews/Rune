#define VMA_IMPLEMENTATION
#include "backend.hh"

#include <cmath>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "core/logger.hh"
#include "utils/vulkan_helpers.hh"

namespace Rune
{
    namespace
    {
#if defined(DEBUG) || !defined(NDEBUG)
        constexpr bool enable_validation_layers = true;
#else
        constexpr bool enable_validation_layers = false;
#endif

        constexpr auto TIMEOUT = 1000000000;

        struct
        {
            vkb::Instance instance;
            vkb::InstanceDispatchTable dispatch;
            // NOTE: This is (kind of) unused now, could be removed if
            // check_available_queues is changed
            vkb::Device device;
        } context;

        struct
        {
            u32 graphics;
            u32 present;
        } queue_family_indices;

        // NOTE: This should probably be moved in some utility header
        //
        // Thanks https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop_code/
        vk::ImageSubresourceRange
        image_subresource_range(vk::ImageAspectFlags aspect_mask)
        {
            // WARN: Default for now from VkGuide
            vk::ImageSubresourceRange sub_image{};

            // XXX: check validity of this
            return sub_image.setAspectMask(aspect_mask)
                .setLevelCount(VK_REMAINING_MIP_LEVELS)
                .setLayerCount(VK_REMAINING_ARRAY_LAYERS);
        }

        void transition_image(vk::CommandBuffer command, VkImage image,
                              vk::ImageLayout current_layout,
                              vk::ImageLayout new_layout)
        {
            vk::ImageMemoryBarrier2 image_barrier{};

            // WARN: Inefficient for now, still figuring this out
            image_barrier
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite
                                  | vk::AccessFlagBits2::eMemoryRead)
                .setOldLayout(current_layout)
                .setNewLayout(new_layout)
                .setSubresourceRange(
                    image_subresource_range(vk::ImageAspectFlags{
                        new_layout == vk::ImageLayout::eDepthAttachmentOptimal
                            ? vk::ImageAspectFlagBits::eDepth
                            : vk::ImageAspectFlagBits::eColor }))
                .setImage(image);

            vk::DependencyInfo dep_info{};
            command.pipelineBarrier2(
                dep_info.setImageMemoryBarrierCount(1).setImageMemoryBarriers(
                    image_barrier));
        }

        // If both extents are equal, maybe use copy instead of blit ?
        void copy_images(vk::CommandBuffer command, vk::Image src,
                         vk::Image dst, vk::Extent2D src_extent,
                         vk::Extent2D dst_extent)
        {
            vk::ImageBlit2 blit{ vk::ImageSubresourceLayers{
                                     vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                                 {},
                                 vk::ImageSubresourceLayers{
                                     vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                                 {} };

            auto [src_x, src_y] = src_extent;
            blit.srcOffsets[1] = vk::Offset3D(src_x, src_y, 1);

            auto [dst_x, dst_y] = dst_extent;
            blit.dstOffsets[1] = vk::Offset3D(dst_x, dst_y, 1);

            vk::BlitImageInfo2 blit_info{ src,
                                          vk::ImageLayout::eTransferSrcOptimal,
                                          dst,
                                          vk::ImageLayout::eTransferDstOptimal,
                                          1,
                                          &blit,
                                          vk::Filter::eLinear };

            command.blitImage2(&blit_info);
        }
    } // namespace

    const RenderData& VulkanRenderer::current_frame_get()
    {
        return frames_[current_frame_ % MAX_IN_FLIGHT];
    }

    void VulkanRenderer::init(Window* window, std::string_view app_name,
                              i32 width, i32 height)
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

        initialized_ = true;

        init_imgui();
    }

    void VulkanRenderer::init_imgui()
    {
        std::array pool_sizes = {
            vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic,
                                    1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 }
        };

        vk::DescriptorPoolCreateInfo pool_info{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        };

        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;

        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        if (device_.createDescriptorPool(&pool_info, nullptr,
                                         &imgui_descriptor_pool_)
            != vk::Result::eSuccess)
        {
            Logger::log(
                Logger::WARN,
                "Failed to initialize ImGui descriptor pools, skipping");
            return;
        }

        ImGui_ImplGlfw_InitForVulkan(window_->get(), true);

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

    bool VulkanRenderer::is_imgui_initialized()
    {
        return imgui_initialized_;
    }

    void VulkanRenderer::imgui_backend_frame()
    {
        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanRenderer::draw_frame()
    {
        imgui_backend_frame();

        auto& current_frame = current_frame_get();
        while (
            device_.waitForFences(current_frame.render_fence, VK_TRUE, TIMEOUT)
            == vk::Result::eTimeout)
            continue;

        device_.resetFences(current_frame.render_fence);
        auto swapchain_image_index =
            device_
                .acquireNextImageKHR(swapchain_, TIMEOUT,
                                     current_frame.swapchain_semaphore)
                .value;

        vk::CommandBuffer command = current_frame.primary_buffer;
        command.reset();

        // FIXME: Right now, simply following VkGuide, integrate customisable
        // draws later

        // NOTE: One time submit -> each frame the buffer is submitted once then
        // reset so might speedup
        command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)));

        transition_image(command, draw_image_.image,
                         vk::ImageLayout::eUndefined,
                         vk::ImageLayout::eGeneral);

        vk::ClearColorValue clear_value;
        float flash = std::abs(std::sin(current_frame_ / 120.f));
        clear_value.setFloat32({ 0.0f, 0.0f, flash, 1.0f });

        auto clear_range =
            image_subresource_range(vk::ImageAspectFlagBits::eColor);

        command.clearColorImage(draw_image_.image, vk::ImageLayout::eGeneral,
                                &clear_value, 1, &clear_range);

        transition_image(command, draw_image_.image, vk::ImageLayout::eGeneral,
                         vk::ImageLayout::eTransferSrcOptimal);

        auto swapchain_image = swapchain_images_[swapchain_image_index];

        transition_image(command, swapchain_image, vk::ImageLayout::eUndefined,
                         vk::ImageLayout::eTransferDstOptimal);

        copy_images(command, draw_image_.image, swapchain_image,
                    draw_image_extent_, swapchain_extent_);

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

        [[unlikely]] if (queue_.submit2(1, &submit_info,
                                        current_frame.render_fence)
                         != vk::Result::eSuccess)
            Logger::log(Logger::WARN, "command submission failed");

        [[unlikely]] if (queue_.presentKHR(vk::PresentInfoKHR{
                             1, &current_frame.render_semaphore, 1, &swapchain_,
                             &swapchain_image_index })
                         != vk::Result::eSuccess)
            Logger::log(Logger::WARN, "queue presentation failed");

        ++current_frame_;
    }

    void VulkanRenderer::cleanup()
    {
        if (!initialized_)
        {
            Logger::log(Logger::WARN,
                        "Attempting to cleanup non-initialized VulkanRenderer");
            return;
        }

        Logger::log(Logger::INFO, "Cleaning up VulkanRenderer");

        vkDeviceWaitIdle(device_);

        device_.destroySwapchainKHR(swapchain_);

        for (auto& image_view : swapchain_images_view_)
            device_.destroyImageView(image_view);

        swapchain_images_view_.clear();
        device_.destroyImageView(draw_image_.image_view);
        vmaDestroyImage(allocator_, draw_image_.image, draw_image_.allocation);

        vkDestroySurfaceKHR(instance_, surface_, nullptr);

        context.dispatch.destroyDebugUtilsMessengerEXT(debug_messenger_,
                                                       nullptr);
        vmaDestroyAllocator(allocator_);

        for (auto& frame : frames_)
        {
            device_.destroyCommandPool(frame.command_pool);
            device_.destroyFence(frame.render_fence);
            device_.destroySemaphore(frame.render_semaphore);
            device_.destroySemaphore(frame.swapchain_semaphore);
        }

        ImGui_ImplVulkan_Shutdown();
        device_.destroyDescriptorPool(imgui_descriptor_pool_);

        device_.destroy();
        instance_.destroy();
    }

    void VulkanRenderer::init_instance(std::string_view app_name)
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

    void VulkanRenderer::create_surface()
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VKCALL(glfwCreateWindowSurface(instance_, window_->get(), nullptr,
                                       &surface));
        surface_ = surface;
        Logger::log(Logger::INFO, "Created Vulkan surface");
    }

    void VulkanRenderer::select_devices()
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

        Logger::log(Logger::INFO, "Device created");
    }

    void VulkanRenderer::check_available_queues() const
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

    void VulkanRenderer::get_queue_indices_and_queue()
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

    void VulkanRenderer::init_allocator()
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

    void VulkanRenderer::init_swapchain(i32 width, i32 height)
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
        image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        image_alloc_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        VkImage image;
        if (vmaCreateImage(allocator_, &image_info, &image_alloc_info, &image,
                           &draw_image_.allocation, nullptr)
            != VK_SUCCESS)
            Logger::abort("Failed to create draw image");

        draw_image_.image = image;

        vk::ImageViewCreateInfo view_info{};
        view_info.setImage(draw_image_.image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(draw_image_.image_format)
            .subresourceRange.setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor);

        if (device_.createImageView(&view_info, nullptr,
                                    &draw_image_.image_view)
            != vk::Result::eSuccess)
            Logger::abort(result.error().message());

        Logger::log(Logger::INFO, "Created swapchain and draw image");
    }

    void VulkanRenderer::create_command_pools_and_buffers()
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

    void VulkanRenderer::init_sync_structs()
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
        }
    }
} // namespace Rune
