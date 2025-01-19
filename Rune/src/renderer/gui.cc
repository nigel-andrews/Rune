#include "gui.hh"

#include "backends/imgui_impl_glfw.h"
#include "imgui.h"

namespace Rune
{
    void Gui::init_context()
    {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
    }

    void Gui::draw_frame()
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        ImGui::Render();
    }

    void Gui::shutdown()
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
} // namespace Rune
