#pragma once

#include <memory>
#include <string_view>

#include "platform/window.hh"
#include "utils/types.hh"

namespace Rune
{
    enum class RenderBackendType;

    class RenderBackend
    {
    public:
        virtual ~RenderBackend() = default;
        virtual void init(Window* window, std::string_view app_name, i32 width,
                          i32 height) = 0;

        virtual RenderBackendType type() = 0;
        virtual bool is_imgui_initialized() = 0;
        virtual void draw_frame() = 0;
        virtual void imgui_frame() = 0;
        virtual void cleanup() = 0;
        // TODO: build shader program function (probably)
    };

    std::unique_ptr<RenderBackend> make_backend(RenderBackendType type);
} // namespace Rune
