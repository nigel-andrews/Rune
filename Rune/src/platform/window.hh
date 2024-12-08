#pragma once

#include <GLFW/glfw3.h>
#include <string_view>

#include "utils/traits.hh"
#include "utils/types.hh"

namespace Rune
{
    class Window : NonCopyable
    {
    public:
        constexpr Window() noexcept = default;
        ~Window();

        void init(std::string_view application_name, i32 width, i32 height);
        void destroy();
        bool should_close();

        constexpr i32 width_get() const
        {
            return width_;
        }

        constexpr i32 height_get() const
        {
            return height_;
        }

        constexpr GLFWwindow* get() const
        {
            return window_;
        }

        GLFWwindow* window_get() const;

    private:
        // NOTE: maybe the window state should be P-Impl
        GLFWwindow* window_ = nullptr;
        i32 width_;
        i32 height_;
    };
} // namespace Rune
