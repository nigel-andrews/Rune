#include "backend.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "core/logger.hh"
#include "utils/vulkan_helpers.hh"

namespace
{
#if defined(DEBUG) || !defined(NDEBUG)
    constexpr bool enable_validation_layers = true;
#else
    constexpr bool enable_validation_layers = false;
#endif

    struct
    {
        vkb::Instance instance;
        vkb::InstanceDispatchTable dispatch;
        vkb::Device device;
    } bootstrap;
} // namespace

namespace Rune
{
    const RenderData& VulkanRenderer::current_frame_get()
    {
        return frames_[current_frame_ % MAX_IN_FLIGHT];
    }

    void VulkanRenderer::init(Window* window, std::string_view app_name,
                              i32 /*width*/, i32 /*height*/)
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
        create_command_pools_and_buffers();

        initialized_ = true;
    }

    void VulkanRenderer::draw_frame()
    {}

    void VulkanRenderer::cleanup()
    {
        if (!initialized_)
        {
            Logger::log(Logger::WARN,
                        "Attempting to cleanup non-initialized VulkanRenderer");
            return;
        }

        Logger::log(Logger::INFO, "Cleaning up VulkanRenderer");

        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        bootstrap.dispatch.destroyDebugUtilsMessengerEXT(debug_messenger_,
                                                         nullptr);

        vkDeviceWaitIdle(device_);

        for (auto& frame : frames_)
            device_.destroyCommandPool(frame.command_pool);

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

        bootstrap.dispatch = builder_return->make_table();

        if (!builder_return)
            Logger::abort(builder_return.error().message());

        debug_messenger_ = builder_return->debug_messenger;

        bootstrap.instance = builder_return.value();
        instance_ = bootstrap.instance.instance;

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
        VkPhysicalDeviceVulkan13Features features{};
        // TODO: investigate more features
        //
        // Maybe this can be retrieved via a function plugged in by the
        // client for more customisation ?
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features.dynamicRendering = true;
        features.synchronization2 = true;

        vkb::PhysicalDeviceSelector selector{ bootstrap.instance };

        auto selected_gpu = selector.set_minimum_version(1, 3)
                                .set_surface(surface_)
                                .require_present()
                                .set_required_features_13(features)
                                .select();

        if (!selected_gpu)
            Logger::abort(selected_gpu.error().message());

        gpu_ = selected_gpu.value();

        Logger::log(Logger::INFO, "Selected GPU :", selected_gpu->name);

        vkb::DeviceBuilder builder{ selected_gpu.value() };
        bootstrap.device = builder.build().value();
        device_ = bootstrap.device.device;

        Logger::log(Logger::INFO, "Device created");
    }

    void VulkanRenderer::check_available_queues() const
    {
        auto graphics_queue =
            bootstrap.device.get_queue(vkb::QueueType::graphics);

        if (!graphics_queue) [[unlikely]]
            Logger::abort(graphics_queue.error().message());
        else [[likely]]
            Logger::log(Logger::INFO, "Found graphics queue");

        auto present_queue =
            bootstrap.device.get_queue(vkb::QueueType::present);

        if (!present_queue) [[unlikely]]
            Logger::abort(present_queue.error().message());
        else [[likely]]
            Logger::log(Logger::INFO, "Found present queue");
    }

    void VulkanRenderer::create_command_pools_and_buffers()
    {
        vk::CommandPoolCreateInfo create_info{
            vk::CommandPoolCreateFlags(),
            *bootstrap.device.get_queue_index(vkb::QueueType::graphics)
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
} // namespace Rune
