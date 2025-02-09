#pragma once

#include "imgui.h"
#include "renderer/render_backend.hh"

namespace Rune
{
    // FIXME: better handling of imgui
    class Gui
    {
    public:
        void init_gui(RenderBackend* backend);
        void draw_frame();
        void shutdown();

    private:
        ImGuiIO* io_;
        RenderBackend* backend_;
    };
} // namespace Rune
