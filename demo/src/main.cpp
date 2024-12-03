#include <Rune/Rune.h>
#include <print>

int main()
{
    Rune::Application app;

    app.start();
    app.run();
    app.stop();

    return 0;
}
