#include "window.hh"

#include <cassert>

#include "GLFW/glfw3.h"
#include "core/logger/logger.hh"
#include "renderer/renderer.hh"

namespace Rune
{
    Window::~Window()
    {
        destroy();
    }

    void Window::init(std::string_view application_name, RenderBackendType type,
                      i32 width, i32 height)
    {
        // TODO: handle multiple backends better
        if (type == RenderBackendType::VULKAN)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            // FIXME:: Remove this when swapchain recreation is available
            glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        }
        else
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

        window_ = glfwCreateWindow(width, height, application_name.data(),
                                   nullptr, nullptr);

        assert(window_);

        width_ = width;
        height_ = height;

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
