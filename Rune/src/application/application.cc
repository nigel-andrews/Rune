#include "application.hh"

#include <print>
#include <stdexcept>

#include "GLFW/glfw3.h"
#include "core/logger.hh"

namespace Rune
{
    void Application::start()
    {
        if (!glfwInit())
            throw std::runtime_error(
                "Application::start: Failed to initialize GLFW");

        window_.init(config_.name, config_.width, config_.height);
        Logger::log(
            Logger::Level::INFO, "Init application with window :", config_.name,
            std::format("of size {}x{}", config_.width, config_.height));
    }

    void Application::config_set(Config config)
    {
        config_ = std::move(config);
    }

    void Application::on_frame_start()
    {}

    void Application::on_frame_end()
    {}

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
        Logger::log(Logger::Level::INFO, "Application shutdown");
    }
} // namespace Rune
