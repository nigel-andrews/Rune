#pragma once

#include "platform/window.hh"
#include "utils/singleton.hh"

namespace Rune
{
    // TODO:: Make this extendable in client code
    class Application : Singleton<Application>
    {
    public:
        void start();
        void run();
        void stop();

        // This returns a pointer so that the user can simply use auto and get a
        // ptr type without having compile errors for forgetting to use refs
        static Application* get()
        {
            return &get_instance();
        }

    private:
        Window window_;
        bool running_;
        // TODO:: time
    };

} // namespace Rune
