#include "application.hh"

#include <print>
#include <stdexcept>

#include "GLFW/glfw3.h"

namespace Rune
{
    void Application::start()
    {
        if (!glfwInit())
            throw std::runtime_error(
                "Application::start: Failed to initialize GLFW");

        window_.init("Rune engine", 800, 600);
    }

    void Application::run()
    {
        while (!window_.should_close())
            glfwPollEvents();
    }

    void Application::stop()
    {
        window_.destroy();
        glfwTerminate();
    }
} // namespace Rune
