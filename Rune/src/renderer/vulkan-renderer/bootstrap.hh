#pragma once

#include <VkBootstrap.h>

namespace Rune::Vulkan
{
    struct BootstrapContext
    {
        vkb::Instance instance;
        vkb::InstanceDispatchTable dispatch;
        // NOTE: This is (kind of) unused now, could be removed if
        // check_available_queues is changed
        vkb::Device device;
    };
} // namespace Rune::Vulkan
