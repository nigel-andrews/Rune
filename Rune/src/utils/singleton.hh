#pragma once

#include "utils/traits.hh"

namespace Rune
{
    template <typename T>
    class Singleton
        : NonCopyable
        , NonMovable
    {
    protected:
        static constexpr T& get_instance()
        {
            static T instance{};
            return instance;
        }

        constexpr Singleton() = default;
        constexpr ~Singleton() = default;
    };
} // namespace Rune
