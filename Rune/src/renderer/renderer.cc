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

        gui_.init_context();
        backend_->init(window, config.name, config.width, config.height);
    }

    void Renderer::draw_frame()
    {
        if (backend_->is_imgui_initialized())
        {
            gui_.draw_frame();
        }

        backend_->draw_frame();
    }

    void Renderer::shutdown()
    {
        backend_->cleanup();
        gui_.shutdown();
    }
} // namespace Rune
