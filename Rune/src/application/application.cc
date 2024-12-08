#include "application.hh"

#include <print>

#include "GLFW/glfw3.h"
#include "core/logger.hh"

namespace Rune
{
    void Application::start()
    {
        if (!glfwInit())
            Logger::abort("Failed to initialize GLFW");
        else
            Logger::log(Logger::INFO, "Initialized GLFW");

        window_.init(config_.name, config_.width, config_.height);
        renderer_.init(RenderBackendType::VULKAN, config_, &window_);

        Logger::log(
            Logger::Level::INFO, "Init application with window :", config_.name,
            std::format("of size {}x{}", config_.width, config_.height));
    }

    void Application::config_set(AppConfig config)
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
        renderer_.shutdown();
        glfwTerminate();
        Logger::log(Logger::Level::INFO, "Application shutdown");
    }
} // namespace Rune
