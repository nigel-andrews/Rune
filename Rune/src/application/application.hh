#pragma once

#include <string>

#include "platform/window.hh"
#include "utils/singleton.hh"

namespace Rune
{
    class Application : Singleton<Application>
    {
    public:
        struct Config
        {
            std::string name = "Application";
            int width = 800;
            int height = 600;
        };

        virtual ~Application() = default;

        void config_set(Config config);

        void start();
        void run();
        void stop();

        virtual void on_frame_start();
        virtual void on_frame_end();

    protected:
        Application() = default;

    private:
        Config config_;
        // FIXME: Renderer should hold window
        Window window_;
        bool running_;
        // TODO:: time
    };
} // namespace Rune

Rune::Application* create_application();
