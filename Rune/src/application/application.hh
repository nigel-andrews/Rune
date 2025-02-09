#pragma once

#include "app_config.hh"
#include "platform/window.hh"
#include "renderer/renderer.hh"
#include "utils/singleton.hh"

namespace Rune
{
    // FIXME: This singleton is probably not required anymore
    class Application : Singleton<Application>
    {
    public:
        virtual ~Application() = default;

        void config_set(AppConfig config);

        void start();
        void run();
        void stop();

        virtual void on_frame_start();
        virtual void on_frame_end();

    protected:
        Application() = default;

    private:
        AppConfig config_;
        Window window_;
        Renderer renderer_;
        bool running_;
        // TODO:: time
    };
} // namespace Rune

Rune::Application* create_application();
