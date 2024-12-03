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

        i32 width_get() const;
        i32 height_get() const;

    private:
        GLFWwindow* window_ = nullptr;
        i32 width_;
        i32 height_;
    };
} // namespace Rune
