#include "application/application.h"

#include <print>

namespace Rune
{
    void Application::init()
    {
        std::println("Application init");
    }

    void Application::update()
    {
        std::println("Application update");
    }

    void Application::cleanup()
    {
        std::println("Application cleanup");
    }
} // namespace Rune
