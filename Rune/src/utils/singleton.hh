#pragma once

#include "utils/traits.hh"

namespace Rune
{
    // There is a remaining flaw : derived classes of T can be constructed and
    // so must either have private constructors (or have a unique entry point
    // controlled by the engine)
    template <typename T>
    class Singleton
        : NonCopyable
        , NonMovable
    {
    protected:
        static constexpr T& get_instance()
        {
            static T instance;

            return instance;
        }

    protected:
        constexpr Singleton() = default;
        constexpr ~Singleton() = default;
    };
} // namespace Rune
