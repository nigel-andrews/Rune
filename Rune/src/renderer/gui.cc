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
        ImGui_ImplGlfw_NewFrame();
        backend_->test_imgui();
    }

    void Gui::shutdown()
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
} // namespace Rune
