#pragma once

#include "renderer/render_backend.hh"
#include "renderer/renderer.hh"

namespace Rune::GL
{
    class Backend final : public RenderBackend
    {
    public:
        void init(Window* window, std::string_view app_name, i32 width,
                  i32 height) final;

        RenderBackendType type() final
        {
            return RenderBackendType::GL;
        }

        bool is_imgui_initialized() final;
        void draw_frame() final;
        void imgui_frame() final;
        void cleanup() final;

    private:
        void init_gl();
        void init_imgui();
        void init_debug_callbacks();
        void init_default_program();

    private:
        Window* window_;
        u32 default_program_;
        u32 vao_;
        u32 vbo_;
        bool imgui_initialized_ = false;
        std::array<float, 4> clear_value_;
        struct
        {
            i32 width;
            i32 height;
        } viewport_;
    };
} // namespace Rune::GL
