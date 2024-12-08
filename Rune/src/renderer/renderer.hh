#pragma once

#include <memory>

#include "application/app_config.hh"
#include "render_backend.hh"

namespace Rune
{
    enum class RenderBackendType
    {
        VULKAN
    };

    class RenderBackend;

    class Renderer
    {
    public:
        void init(RenderBackendType type, const AppConfig& config,
                  Window* window);
        // TODO: Resize
        void draw_frame();
        void shutdown();

    private:
        std::unique_ptr<RenderBackend> backend_;
    };
} // namespace Rune
