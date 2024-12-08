#include "window.hh"

#include <cassert>

#include "GLFW/glfw3.h"
#include "core/logger.hh"

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
        assert(window_);

        Logger::log(Logger::INFO, "Initialized window");
    }

    bool Window::should_close()
    {
        return glfwWindowShouldClose(window_);
    }

    void Window::destroy()
    {
        if (!window_)
            return;

        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
} // namespace Rune
