#include "backend.hh"

#include "core/logger.hh"

namespace Rune
{
    void VulkanRenderer::init()
    {
        Logger::log(Logger::INFO, "Initializing VulkanRenderer");
    }

    void VulkanRenderer::draw_frame()
    {
        Logger::log(Logger::INFO, "Draw frame called");
    }

    void VulkanRenderer::cleanup()
    {
        Logger::log(Logger::INFO, "Cleaning up VulkanRenderer");
    }
} // namespace Rune
