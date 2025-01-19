#pragma once

#include "imgui.h"
#include "renderer/render_backend.hh"

namespace Rune
{
    class Gui
    {
    public:
        void init_context();
        void draw_frame();
        void shutdown();

    private:
        ImGuiIO* io_;
        RenderBackend* backend_;
    };
} // namespace Rune
