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
            struct ConstructGuard final : T
            {
                void prevent_construction() const noexcept override
                {}
            };

            static ConstructGuard instance{};
            return instance;
        }

    protected:
        constexpr Singleton() = default;
        constexpr ~Singleton() = default;

    private:
        virtual void prevent_construction() const noexcept = 0;
    };
} // namespace Rune
