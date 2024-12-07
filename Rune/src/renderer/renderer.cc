#include "renderer.hh"

#include <memory>

#if 1
#    include "vulkan/backend.hh"
#endif

namespace Rune
{
    void Renderer::init(RenderBackendType type)
    {
        switch (type)
        {
        case RenderBackendType::VULKAN:
            backend_ = std::make_unique<VulkanRenderer>();
            break;
        }

        backend_->init();
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
