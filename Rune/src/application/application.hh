#pragma once

#include "platform/window.hh"
#include "utils/singleton.hh"

namespace Rune
{
    class Application : Singleton<Application>
    {
    public:
        ~Application() = default;

        void start();
        void run();
        void stop();

        static Application* get()
        {
            return &get_instance();
        }

        friend Singleton;

    private:
        Application() = default;

        Window window_;
        bool running_;
        // TODO:: time
    };

} // namespace Rune
