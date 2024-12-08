#include "backend.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#include "core/logger.hh"
#include "utils/vulkan_helpers.hh"

namespace
{
#if defined(DEBUG) || !defined(NDEBUG)
    constexpr bool enable_validation_layers = true;
#else
    constexpr bool enable_validation_layers = false;
#endif
} // namespace

namespace Rune
{
    void VulkanRenderer::init(Window* window, std::string_view app_name,
                              i32 /*width*/, i32 /*height*/)
    {
        Logger::log(Logger::INFO, "Initializing VulkanRenderer");

        window_ = window;

        auto vkb_instance = init_instance(app_name);
        create_surface();
        select_physical_device(vkb_instance);
    }

    void VulkanRenderer::draw_frame()
    {}

    void VulkanRenderer::cleanup()
    {
        Logger::log(Logger::INFO, "Cleaning up VulkanRenderer");

        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        dispatch_.destroyDebugUtilsMessengerEXT(debug_messenger_, nullptr);

        instance_.destroy();
    }

    vkb::Instance VulkanRenderer::init_instance(std::string_view app_name)
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

        dispatch_ = builder_return->make_table();

        if (!builder_return)
            Logger::abort(builder_return.error().message());

        debug_messenger_ = builder_return->debug_messenger;

        auto vkb_instance = builder_return.value();
        instance_ = vkb_instance;

        Logger::log(Logger::INFO, "Created Vulkan instance");

        return vkb_instance;
    }

    void VulkanRenderer::create_surface()
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VKCALL(glfwCreateWindowSurface(instance_, window_->get(), nullptr,
                                       &surface));
        surface_ = surface;
        Logger::log(Logger::INFO, "Created Vulkan surface");
    }

    void VulkanRenderer::select_physical_device(vkb::Instance& vkb_instance)
    {
        VkPhysicalDeviceVulkan13Features features{};
        // TODO: investigate more features
        //
        // Maybe this can be retrieved via a function plugged in by the
        // client for more customisation ?
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features.dynamicRendering = true;
        features.synchronization2 = true;

        vkb::PhysicalDeviceSelector selector{ vkb_instance };

        auto selected_gpu = selector.set_minimum_version(1, 3)
                                .set_surface(surface_)
                                .require_present()
                                .set_required_features_13(features)
                                .select();

        if (!selected_gpu)
            Logger::abort(selected_gpu.error().message());

        gpu_ = selected_gpu.value();
        Logger::log(Logger::INFO, "Selected GPU :", selected_gpu->name);
    }
} // namespace Rune
