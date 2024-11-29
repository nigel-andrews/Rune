#pragma once

namespace Rune
{
    // TODO: Virtual interface to extend app into others
    class Application
    {
    public:
        Application() = default;
        ~Application() = default;

        void init();
        void update();
        void cleanup();
    };
} // namespace Rune
