#include "window.hh"

namespace Rune
{
    Window::~Window()
    {
        destroy();
    }

    void Window::init(std::string_view application_name, i32 width, i32 height)
    {
        // TODO:: Vulkan only for now, need to handle OpenGL
        // (Compile two different implementation in function of renderer)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // FIXME:: Remove this when swapchain recreation is available
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        window_ = glfwCreateWindow(width, height, application_name.data(),
                                   nullptr, nullptr);
    }

    void Window::destroy()
    {
        if (!window_)
            return;

        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
} // namespace Rune
