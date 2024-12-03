#pragma once

#include "utils/traits.h"

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
    };
} // namespace Rune
