#pragma once

#include "platform/window.hh"

namespace Rune
{
    class Application
    {
    public:
        virtual ~Application() = default;

        void start();
        void run();
        void stop();

        virtual void on_frame_start();
        virtual void on_frame_end();

    private:
        static bool exists_;

        // FIXME: Renderer should hold window
        Window window_;
        bool running_;
        // TODO:: time
    };
} // namespace Rune

Rune::Application* create_application();
