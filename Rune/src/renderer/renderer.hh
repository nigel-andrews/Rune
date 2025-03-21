#pragma once

#include <memory>

#include "application/app_config.hh"
#include "gui.hh"
#include "render_backend.hh"

namespace Rune
{
    enum class RenderBackendType
    {
        VULKAN,
        GL,
    };

    class Renderer
    {
    public:
        void init(RenderBackendType type, const AppConfig& config,
                  Window* window);
        // TODO: Resize
        void draw_frame();
        void shutdown();
        RenderBackendType type_get() const;

    private:
        std::unique_ptr<RenderBackend> backend_;
        Gui gui_;
    };
} // namespace Rune
