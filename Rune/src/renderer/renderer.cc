#include "renderer.hh"

#include <memory>

#include "renderer/render_backend.hh"

namespace Rune
{
    void Renderer::init(RenderBackendType type, const AppConfig& config,
                        Window* window)
    {
        backend_ = make_backend(type);
        gui_.init_gui(backend_.get());
        backend_->init(window, config.name, config.width, config.height);
    }

    RenderBackendType Renderer::type_get() const
    {
        return backend_->type();
    }

    void Renderer::draw_frame()
    {
        if (backend_->is_imgui_initialized())
        {
            gui_.draw_frame();
        }

        backend_->draw_frame();
    }

    void Renderer::shutdown()
    {
        backend_->cleanup();
        gui_.shutdown();
    }
} // namespace Rune
