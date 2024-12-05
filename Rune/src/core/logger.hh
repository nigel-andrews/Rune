#pragma once

#include <array>
#include <iostream>

namespace Rune::Logger
{
    enum Level
    {
        FATAL = 0,
        INFO,
        WARN,
        ERROR,
    };

    // TODO: color output
    template <typename... Ts>
    constexpr void log(Level log_level, Ts&&... args)
    {
#if defined(DEBUG) || !defined(NDEBUG)
        static constexpr std::array string_repr{
            "FATAL",
            "INFO",
            "WARN",
            "ERROR",
        };
        std::print(std::clog, "[{}]:", string_repr[log_level]);
        ((std::clog << " " << std::forward<Ts>(args)), ...) << std::endl;
#endif
    }
} // namespace Rune::Logger
