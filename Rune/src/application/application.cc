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

    void Application::on_frame_start()
    {
        std::println("Starting frame");
    }

    void Application::on_frame_end()
    {
        std::println("Ending frame");
    }

    void Application::run()
    {
        while (!window_.should_close())
        {
            // Handle messages
            glfwPollEvents();

            on_frame_start();

            // TODO: Render

            on_frame_end();
        }
    }

    void Application::stop()
    {
        window_.destroy();
        glfwTerminate();
    }
} // namespace Rune
