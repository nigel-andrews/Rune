#pragma once

#include "platform/window.hh"
#include "utils/traits.hh"

namespace Rune
{
    class Application : NonCopyable
    {
    public:
        Application() = default;
        ~Application() = default;

        void start();
        void run();
        void stop();

    private:
        // Window window_;
    };
} // namespace Rune
