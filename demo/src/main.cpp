#include <Rune/Rune.h>
#include <print>

int main()
{
    auto app = Rune::Application::get();

    app->start();
    app->run();
    app->stop();

    return 0;
}
