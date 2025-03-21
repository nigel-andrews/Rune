#include "render_backend.hh"

#include <memory>
#include <utility>

#include "vulkan-renderer/backend.hh"
// #include "opengl-renderer/backend.hh"
#include "renderer/renderer.hh"

namespace Rune
{
    std::unique_ptr<RenderBackend> make_backend(RenderBackendType type)
    {
        switch (type)
        {
        case Rune::RenderBackendType::VULKAN:
            return std::make_unique<Vulkan::Backend>();
            // case Rune::RenderBackendType::GL:
            //     return std::make_unique<GL::Backend>();
        }

        std::unreachable();
    }
} // namespace Rune
