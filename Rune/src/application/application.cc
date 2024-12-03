#include "application.hh"

#include <print>

namespace Rune
{
    void Application::start()
    {
        std::println("Starting engine");
    }

    void Application::run()
    {
        std::println("Running engine");
    }

    void Application::stop()
    {
        std::println("Stopping engine");
    }
} // namespace Rune
