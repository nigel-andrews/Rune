#pragma once

#include <memory>

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
        void init(RenderBackendType type);
        // TODO: Resize
        void draw_frame();
        void shutdown();

    private:
        std::unique_ptr<RenderBackend> backend_;
    };
} // namespace Rune
