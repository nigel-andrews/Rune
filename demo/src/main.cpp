#include <Rune/Rune.h>
#include <memory>
#include <print>

#include "application/application.hh"

class DemoApplication final : public Rune::Application
{
public:
    DemoApplication() = default;
    ~DemoApplication() final = default;

    virtual void on_frame_start() final
    {
        std::println("Demo frame start");
    }

    virtual void on_frame_end() final
    {
        std::println("Demo frame end");
    }
};

Rune::Application* create_application()
{
    return new DemoApplication();
}

int main()
{
    auto app = std::unique_ptr<Rune::Application>(create_application());

    app->start();
    app->run();
    app->stop();

    return 0;
}
