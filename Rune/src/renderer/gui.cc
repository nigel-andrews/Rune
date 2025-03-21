#include "gui.hh"

#include "backends/imgui_impl_glfw.h"
#include "imgui.h"

namespace Rune
{
    void Gui::init_gui(RenderBackend* backend)
    {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        backend_ = backend;
    }

    void Gui::draw_frame()
    {
        // FIXME: this is awkward to use whether with Vulkan or GL.
        // backend_->imgui_frame();

        // The code should call begin imgui frame per backend and do the draw
        // commands and render, then draw_frame should handle the per backend
        // render draw data and submission
    }

    void Gui::shutdown()
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
} // namespace Rune
