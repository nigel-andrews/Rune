#include "renderer.hh"

#include <memory>

// FIXME: Defines
#if 1
#    include "vulkan-renderer/backend.hh"
#endif

namespace Rune
{
    void Renderer::init(RenderBackendType type, const AppConfig& config,
                        Window* window)
    {
        switch (type)
        {
        case RenderBackendType::VULKAN:
            backend_ = std::make_unique<VulkanRenderer>();
            break;
        }

        backend_->init(window, config.name, config.width, config.height);
        // TODO: init ImGui in function of the backend
    }

    void Renderer::draw_frame()
    {
        backend_->draw_frame();
    }

    void Renderer::shutdown()
    {
        backend_->cleanup();
    }
} // namespace Rune
