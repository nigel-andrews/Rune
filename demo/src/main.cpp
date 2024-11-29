#include <Rune/Rune.h>
#include <print>

int main()
{
    Rune::Application app;

    app.init();
    app.update();
    app.cleanup();

    return 0;
}
